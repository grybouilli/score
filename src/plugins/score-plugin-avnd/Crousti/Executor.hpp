#pragma once

#include <Process/Execution/ProcessComponent.hpp>
#include <Process/ExecutionContext.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <Crousti/CpuAnalysisNode.hpp>
#include <Crousti/CpuFilterNode.hpp>
#include <Crousti/CpuGeneratorNode.hpp>
#include <Crousti/GpuComputeNode.hpp>
#include <Crousti/GpuNode.hpp>
#include <Crousti/MessageBus.hpp>
#include <Crousti/Metadatas.hpp>
#include <Crousti/ProcessModel.hpp>
#include <Engine/Node/TickPolicy.hpp>

#include <score/tools/Bind.hpp>

#include <ossia/dataflow/exec_state_facade.hpp>
#include <ossia/dataflow/node_process.hpp>

#include <ossia-qt/invoke.hpp>

#include <QGuiApplication>

#if SCORE_PLUGIN_GFX
#include <Crousti/GpuNode.hpp>
#include <Gfx/GfxApplicationPlugin.hpp>
#endif

#include <Media/AudioDecoder.hpp>

#include <score/tools/ThreadPool.hpp>

#include <QTimer>

#include <avnd/binding/ossia/data_node.hpp>
#include <avnd/binding/ossia/mono_audio_node.hpp>
#include <avnd/binding/ossia/node.hpp>
#include <avnd/binding/ossia/ossia_audio_node.hpp>
#include <avnd/binding/ossia/poly_audio_node.hpp>
#include <avnd/concepts/ui.hpp>
#include <avnd/concepts/worker.hpp>
#include <libremidi/reader.hpp>

namespace oscr
{
namespace
{

static QString
filenameFromPort(const ossia::value& value, const score::DocumentContext& ctx)
{
  if(auto str = value.target<std::string>())
    return score::locateFilePath(QString::fromStdString(*str).trimmed(), ctx);
  return {};
}

// TODO refactor this into a generic explicit soundfile loaded mechanism
static auto loadSoundfile(
    Process::ControlInlet* inlet, const score::DocumentContext& ctx, double rate)
{
  // Initialize the control with the current soundfile
  if(auto str = filenameFromPort(inlet->value(), ctx); !str.isEmpty())
  {
    auto dec = Media::AudioDecoder::decode_synchronous(str, rate);

    if(dec.has_value())
    {
      auto hdl = std::make_shared<ossia::audio_data>();
      hdl->data = std::move(dec->second);
      hdl->path = str.toStdString();
      hdl->rate = rate;
      return hdl;
    }
  }
  return ossia::audio_handle{};
}

using midifile_handle = std::shared_ptr<oscr::midifile_data>;
inline midifile_handle
loadMidifile(Process::ControlInlet* inlet, const score::DocumentContext& ctx)
{
  // Initialize the control with the current soundfile
  if(auto str = filenameFromPort(inlet->value(), ctx); !str.isEmpty())
  {
    QFile f(str);
    if(!f.open(QIODevice::ReadOnly))
      return {};
    auto ptr = f.map(0, f.size());

    auto hdl = std::make_shared<oscr::midifile_data>();
    if(auto ret = hdl->reader.parse((uint8_t*)ptr, f.size());
       ret == libremidi::reader::invalid)
      return {};

    hdl->filename = str.toStdString();
    return hdl;
  }
  return {};
}

using raw_file_handle = std::shared_ptr<raw_file_data>;
inline raw_file_handle loadRawfile(
    Process::ControlInlet* inlet, const score::DocumentContext& ctx, bool text,
    bool mmap)
{
  // Initialize the control with the current soundfile
  if(auto filename = filenameFromPort(inlet->value(), ctx); !filename.isEmpty())
  {
    if(!QFile::exists(filename))
      return {};

    auto hdl = std::make_shared<oscr::raw_file_data>();
    hdl->file.setFileName(filename);
    if(!hdl->file.open(QIODevice::ReadOnly))
      return {};

    if(mmap)
    {
      auto map = (char*)hdl->file.map(0, hdl->file.size());
      hdl->data = QByteArray::fromRawData(map, hdl->file.size());
    }
    else
    {
      if(text)
        hdl->file.setTextModeEnabled(true);

      hdl->data = hdl->file.readAll();
    }
    hdl->filename = filename.toStdString();
    return hdl;
  }
  return {};
}

inline auto loadSoundfile(
    Process::ControlInlet* inlet, const score::DocumentContext& ctx,
    const std::shared_ptr<ossia::execution_state>& st)
{
  const double rate = ossia::exec_state_facade{st.get()}.sampleRate();
  return loadSoundfile(inlet, ctx, rate);
}
}

template <typename ExecNode_T, typename T, std::size_t ControlN>
struct control_updater
{
  std::weak_ptr<ExecNode_T> weak_node;
  T v;
  void operator()()
  {
    if(auto n = weak_node.lock())
    {
      n->template control_updated_from_ui<T, ControlN>(std::move(v));
    }
  }
};

template <typename Node>
struct setup_Impl0
{
  using ExecNode = safe_node<Node>;
  using Model = ProcessModel<Node>;

  Model& element;
  const Execution::Context& ctx;
  const std::shared_ptr<ExecNode>& node_ptr;
  QObject* parent;

  template <typename Field, std::size_t NPred, std::size_t NField>
  struct con_unvalidated
  {
    const Execution::Context& ctx;
    std::weak_ptr<ExecNode> weak_node;
    Field& field;
    void operator()(const ossia::value& val)
    {
      constexpr auto control_index = NPred;

      using control_value_type = std::decay_t<decltype(Field::value)>;

      if(auto node = weak_node.lock())
      {
        control_value_type v;
        node->from_ossia_value(field, val, v, avnd::field_index<NField>{});
        ctx.executionQueue.enqueue(
            control_updater<ExecNode, control_value_type, control_index>{
                weak_node, std::move(v)});
      }
    }
  };

  template <typename Field, std::size_t N, std::size_t NField>
  constexpr void
  operator()(Field& param, avnd::predicate_index<N> np, avnd::field_index<NField> nf)
  {
    if(auto inlet
       = dynamic_cast<Process::ControlInlet*>(modelPort<Node>(element.inlets(), NField)))
    {
      // Initialize the control with the current value of the inlet if it is not an optional
      if constexpr(!requires { param.value.reset(); })
        node_ptr->from_ossia_value(param, inlet->value(), param.value, nf);

      {
        avnd::effect_container<Node>& eff = node_ptr->impl;
        {
          for(auto& state : eff.full_state())
          {
            if_possible(param.update(state.effect));
          }
        }
      }

      // Connect to changes
      std::weak_ptr<ExecNode> weak_node = node_ptr;
      QObject::connect(
          inlet, &Process::ControlInlet::valueChanged, parent,
          con_unvalidated<Field, N, NField>{ctx, weak_node, param});
    }
    // Else it's an unhandled value inlet
  }

  template <avnd::soundfile_port Field, std::size_t N, std::size_t NField>
  void operator()(Field& param, avnd::predicate_index<N>, avnd::field_index<NField>)
  {
    auto inlet
        = safe_cast<Process::ControlInlet*>(modelPort<Node>(element.inlets(), NField));

    // First we can load it directly since execution hasn't started yet
    if(auto hdl = loadSoundfile(inlet, ctx.doc, ctx.execState))
      node_ptr->soundfile_loaded(
          hdl, avnd::predicate_index<N>{}, avnd::field_index<NField>{});

    // Connect to changes
    std::weak_ptr<ExecNode> weak_node = node_ptr;
    std::weak_ptr<ossia::execution_state> weak_st = ctx.execState;
    QObject::connect(
        inlet, &Process::ControlInlet::valueChanged, parent,
        [inlet, &ctx = this->ctx, weak_node = std::move(weak_node),
         weak_st = std::move(weak_st)] {
      if(auto n = weak_node.lock())
        if(auto st = weak_st.lock())
          if(auto file = loadSoundfile(inlet, ctx.doc, st))
          {
            ctx.executionQueue.enqueue([f = std::move(file), weak_node]() mutable {
              auto n = weak_node.lock();
              if(!n)
                return;

              // We store the sound file handle returned in this lambda so that it gets
              // GC'd in the main thread
              n->soundfile_loaded(
                  f, avnd::predicate_index<N>{}, avnd::field_index<NField>{});
            });
          }
        });
  }

  template <avnd::midifile_port Field, std::size_t N, std::size_t NField>
  void operator()(Field& param, avnd::predicate_index<N>, avnd::field_index<NField>)
  {
    auto inlet
        = safe_cast<Process::ControlInlet*>(modelPort<Node>(element.inlets(), NField));

    // First we can load it directly since execution hasn't started yet
    if(auto hdl = loadMidifile(inlet, ctx.doc))
      node_ptr->midifile_loaded(
          hdl, avnd::predicate_index<N>{}, avnd::field_index<NField>{});

    // Connect to changes
    std::weak_ptr<ExecNode> weak_node = node_ptr;
    std::weak_ptr<ossia::execution_state> weak_st = ctx.execState;
    QObject::connect(
        inlet, &Process::ControlInlet::valueChanged, parent,
        [inlet, &ctx = this->ctx, weak_node = std::move(weak_node)] {
      if(auto n = weak_node.lock())
        if(auto file = loadMidifile(inlet, ctx.doc))
        {
          ctx.executionQueue.enqueue([f = std::move(file), weak_node]() mutable {
            auto n = weak_node.lock();
            if(!n)
              return;

            // We store the sound file handle returned in this lambda so that it gets
            // GC'd in the main thread
            n->midifile_loaded(
                f, avnd::predicate_index<N>{}, avnd::field_index<NField>{});
          });
        }
        });
  }

  template <typename Field>
  static auto executePortPreprocess(auto& file)
  {
    using field_file_type = decltype(Field::file);
    field_file_type ffile;
    ffile.bytes = decltype(ffile.bytes)(file.data.constData(), file.file.size());
    ffile.filename = file.filename;
    return Field::process(ffile);
  }
  template <avnd::raw_file_port Field, std::size_t N, std::size_t NField>
  void operator()(Field& param, avnd::predicate_index<N>, avnd::field_index<NField>)
  {
    auto inlet
        = safe_cast<Process::ControlInlet*>(modelPort<Node>(element.inlets(), NField));

    using file_ports = avnd::raw_file_input_introspection<Node>;
    using elt = typename file_ports::template nth_element<N>;
    constexpr bool has_text = requires { decltype(elt::file)::text; };
    constexpr bool has_mmap = requires { decltype(elt::file)::mmap; };

    // First we can load it directly since execution hasn't started yet
    if(auto hdl = loadRawfile(inlet, ctx.doc, has_text, has_mmap))
    {
      if constexpr(avnd::port_can_process<Field>)
      {
        // FIXME also do it when we get a run-time message from the exec engine,
        // OSC, etc
        auto func = executePortPreprocess<Field>(*hdl);
        node_ptr->file_loaded(
            hdl, avnd::predicate_index<N>{}, avnd::field_index<NField>{});
        if(func)
          func(node_ptr->impl.effect);
      }
      else
      {
        node_ptr->file_loaded(
            hdl, avnd::predicate_index<N>{}, avnd::field_index<NField>{});
      }
    }

    // Connect to changes
    std::weak_ptr<ExecNode> weak_node = node_ptr;
    std::weak_ptr<ossia::execution_state> weak_st = ctx.execState;
    QObject::connect(
        inlet, &Process::ControlInlet::valueChanged, parent,
        [inlet, &ctx = this->ctx, weak_node = std::move(weak_node)] {
      if(auto n = weak_node.lock())
        if(auto file = loadRawfile(inlet, ctx.doc, has_text, has_mmap))
        {
          if constexpr(avnd::port_can_process<Field>)
          {
            auto func = executePortPreprocess<Field>(*file);
            ctx.executionQueue.enqueue(
                [f = std::move(file), weak_node, ff = std::move(func)]() mutable {
              auto n = weak_node.lock();
              if(!n)
                return;

              // We store the sound file handle returned in this lambda so that it gets
              // GC'd in the main thread
              n->file_loaded(f, avnd::predicate_index<N>{}, avnd::field_index<NField>{});
              if(ff)
                ff(n->impl.effect);
            });
          }
          else
          {
            ctx.executionQueue.enqueue([f = std::move(file), weak_node]() mutable {
              auto n = weak_node.lock();
              if(!n)
                return;

              // We store the sound file handle returned in this lambda so that it gets
              // GC'd in the main thread
              n->file_loaded(f, avnd::predicate_index<N>{}, avnd::field_index<NField>{});
            });
          }
        }
        });
  }
};

template <typename Node>
struct ApplyEngineControlChangeToUI
{
  using ExecNode = safe_node<Node>;
  using Model = ProcessModel<Node>;

  typename ExecNode::control_input_values_type& arr;
  Model& element;

  template <std::size_t N, std::size_t NField>
  void operator()(auto& field, avnd::predicate_index<N>, avnd::field_index<NField>)
  {
    if(auto p = modelPort<Node>(element.inlets(), NField))
      if(auto inlet = dynamic_cast<Process::ControlInlet*>(p))
        inlet->setExecutionValue(oscr::to_ossia_value(field, field.value));
  }
};

template <typename Node>
struct setup_Impl1_Out
{
  using ExecNode = safe_node<Node>;
  using Model = ProcessModel<Node>;
  typename ExecNode::control_output_values_type& arr;
  Model& element;

  template <std::size_t N, std::size_t NField>
  void operator()(auto& field, avnd::predicate_index<N>, avnd::field_index<NField>)
  {
    auto outlet
        = safe_cast<Process::ControlOutlet*>(modelPort<Node>(element.outlets(), NField));
    outlet->setValue(oscr::to_ossia_value(field, field.value));
  }
};

template <typename Node>
struct ExecutorGuiUpdate
{
  using ExecNode = safe_node<Node>;
  using Model = ProcessModel<Node>;
  std::weak_ptr<ExecNode> weak_node;
  Model& element;

  void handle_controls(ExecNode& node) const noexcept
  {
    using namespace ossia::safe_nodes;
    // TODO disconnect the connection ? it will be disconnected shortly
    // after...

    typename ExecNode::control_input_values_type arr;
    bool ok = false;
    while(node.control.ins_queue.try_dequeue(arr))
    {
      ok = true;
    }
    if(ok)
    {
      for(auto& state : node.impl.full_state())
      {
        avnd::control_input_introspection<Node>::for_all_n2(
            state.inputs, ApplyEngineControlChangeToUI<Node>{arr, element});
      }
    }
  }

  void handle_control_outs(ExecNode& node) const noexcept
  {
    using namespace ossia::safe_nodes;
    // TODO disconnect the connection ? it will be disconnected shortly
    // after...
    typename ExecNode::control_output_values_type arr;
    bool ok = false;
    while(node.control.outs_queue.try_dequeue(arr))
    {
      ok = true;
    }
    if(ok)
    {
      avnd::control_output_introspection<Node>::for_all_n2(
          avnd::get_outputs<Node>(node.impl), setup_Impl1_Out<Node>{arr, element});
    }
  }

  void operator()() const noexcept
  {
    if(auto node = weak_node.lock())
    {
      constexpr const auto control_count = avnd::control_input_introspection<Node>::size;
      constexpr const auto control_out_count
          = avnd::control_output_introspection<Node>::size;
      if constexpr(control_count > 0)
        handle_controls(*node);

      if constexpr(control_out_count > 0)
        handle_control_outs(*node);
    }
  }
};

template <typename T, bool Predicate>
struct type_if;
template <typename T>
struct type_if<T, false>
{
  type_if() = default;
  type_if(const type_if&) = default;
  type_if(type_if&&) = default;
  type_if& operator=(const type_if&) = default;
  type_if& operator=(type_if&&) = default;

  template <typename U>
  type_if(U&&)
  {
  }
  template <typename U>
  T& operator=(U&& u) noexcept
  {
    return *this;
  }
};

template <typename T>
struct type_if<T, true>
{
  [[no_unique_address]] T value;

  type_if() = default;
  type_if(const type_if&) = default;
  type_if(type_if&&) = default;
  type_if& operator=(const type_if&) = default;
  type_if& operator=(type_if&&) = default;

  template <typename U>
  type_if(U&& other)
      : value{std::forward<U>(other)}
  {
  }

  operator const T&() const noexcept { return value; }
  operator T&() noexcept { return value; }
  operator T&&() && noexcept { return std::move(value); }

  template <typename U>
  T& operator=(U&& u) noexcept
  {
    return value = std::forward<U>(u);
  }
};

template <typename Node>
class Executor final
    : public Execution::ProcessComponent_T<ProcessModel<Node>, ossia::node_process>
{
public:
  static Q_DECL_RELAXED_CONSTEXPR UuidKey<score::Component> static_key() noexcept
  {
    return uuid_from_string<Node>();
  }

  UuidKey<score::Component> key() const noexcept final override { return static_key(); }

  bool key_match(UuidKey<score::Component> other) const noexcept final override
  {
    return static_key() == other || Execution::ProcessComponent::base_key_match(other);
  }

  [[no_unique_address]] type_if<int, is_gpu<Node>> node_id = -1;

  Executor(ProcessModel<Node>& element, const ::Execution::Context& ctx, QObject* p)
      : Execution::ProcessComponent_T<ProcessModel<Node>, ossia::node_process>{
          element, ctx, "Executor::ProcessModel<Info>", p}
  {
    const auto id
        = std::hash<ObjectPath>{}(Path<Process::ProcessModel>{element}.unsafePath());
#if SCORE_PLUGIN_GFX
    if constexpr(is_gpu<Node>)
    {
      auto& gfx_exec = ctx.doc.plugin<Gfx::DocumentPlugin>().exec;

      // Create the executor in the audio thread
      auto node = std::make_shared<Gfx::gfx_exec_node>(gfx_exec);
      node->prepare(*ctx.execState);

      this->node = node;

      // Create the controls, inputs outputs etc.
      std::size_t i = 0;
      for(auto& ctl : element.inlets())
      {
        if(auto ctrl = qobject_cast<Process::ControlInlet*>(ctl))
        {
          auto& p = node->add_control();
          p->value = ctrl->value();
          p->changed = true;

          QObject::connect(
              ctrl, &Process::ControlInlet::valueChanged, this,
              Gfx::con_unvalidated{ctx, i, 0, node});
          i++;
        }
        else if(auto ctrl = qobject_cast<Process::ValueInlet*>(ctl))
        {
          auto& p = node->add_control();
          p->changed = true;
          i++;
        }
        else if(auto ctrl = qobject_cast<Process::AudioInlet*>(ctl))
        {
          node->add_audio();
        }
        else if(auto ctrl = qobject_cast<Gfx::TextureInlet*>(ctl))
        {
          node->add_texture();
        }
      }

      // FIXME refactor this with other GFX processes
      for(auto* outlet : element.outlets())
      {
        if(auto ctrl = qobject_cast<Process::ControlOutlet*>(outlet))
        {
          node->add_control_out();
        }
        else if(auto ctrl = qobject_cast<Process::ValueOutlet*>(outlet))
        {
          node->add_control_out();
        }
        else if(auto out = qobject_cast<Gfx::TextureOutlet*>(outlet))
        {
          node->add_texture_out();
          out->nodeId = node_id;
        }
      }

      // Create the GPU node

      std::weak_ptr qex_ptr = std::shared_ptr<Execution::ExecutionCommandQueue>(
          ctx.alias.lock(), &ctx.executionQueue);
      std::unique_ptr<score::gfx::Node> ptr;
      if constexpr(GpuGraphicsNode2<Node>)
      {
        ptr.reset(new CustomGpuNode<Node>(qex_ptr, node->control_outs, id));
      }
      else if constexpr(GpuComputeNode2<Node>)
      {
        ptr.reset(new GpuComputeNode<Node>(qex_ptr, node->control_outs, id));
      }
      else if constexpr(GpuNode<Node>)
      {
        ptr.reset(new GfxNode<Node>(qex_ptr, node->control_outs, id));
      }
      node->id = gfx_exec.ui->register_node(std::move(ptr));
      node_id = node->id;
    }
    else
#endif
    {
      auto st = ossia::exec_state_facade{ctx.execState.get()};
      std::shared_ptr<safe_node<Node>> ptr;
      auto node = new safe_node<Node>{st.bufferSize(), (double)st.sampleRate(), id};
      node->prepare(*ctx.execState.get()); // Preparation of the ossia side

      if_possible(node->impl.effect.ossia_state = st);
      ptr.reset(node);
      this->node = ptr;

      connect_message_bus(element, ctx, ptr);
      connect_worker(element, ctx, ptr);

      node->finish_init();

      connect_controls(element, ctx, ptr);
      update_controls(ptr);

      // To call prepare() after evertyhing is ready
      node->audio_configuration_changed();
    }

    this->m_ossia_process = std::make_shared<ossia::node_process>(this->node);
  }

  void connect_controls(
      ProcessModel<Node>& element, const ::Execution::Context& ctx,
      std::shared_ptr<safe_node<Node>>& ptr)
  {
    using control_inputs_type = avnd::control_input_introspection<Node>;
    using curve_inputs_type = avnd::curve_input_introspection<Node>;
    using soundfile_inputs_type = avnd::soundfile_input_introspection<Node>;
    using midifile_inputs_type = avnd::midifile_input_introspection<Node>;
    using raw_file_inputs_type = avnd::raw_file_input_introspection<Node>;
    using control_outputs_type = avnd::control_output_introspection<Node>;

    // UI controls to engine
    avnd::effect_container<Node>& eff = ptr->impl;

    if constexpr(control_inputs_type::size > 0)
    {
      // Initialize all the controls in the node with the current value.
      // And update the node when the UI changes

      for(auto& state : eff.full_state())
      {
        control_inputs_type::for_all_n2(
            state.inputs, setup_Impl0<Node>{element, ctx, ptr, this});
      }
    }
    if constexpr(curve_inputs_type::size > 0)
    {
      // Initialize all the controls in the node with the current value.
      // And update the node when the UI changes

      for(auto& state : eff.full_state())
      {
        curve_inputs_type::for_all_n2(
            state.inputs, setup_Impl0<Node>{element, ctx, ptr, this});
      }
    }
    if constexpr(soundfile_inputs_type::size > 0)
    {
      soundfile_inputs_type::for_all_n2(
          avnd::get_inputs<Node>(eff), setup_Impl0<Node>{element, ctx, ptr, this});
    }
    if constexpr(midifile_inputs_type::size > 0)
    {
      midifile_inputs_type::for_all_n2(
          avnd::get_inputs<Node>(eff), setup_Impl0<Node>{element, ctx, ptr, this});
    }
    if constexpr(raw_file_inputs_type::size > 0)
    {
      raw_file_inputs_type::for_all_n2(
          avnd::get_inputs<Node>(eff), setup_Impl0<Node>{element, ctx, ptr, this});
    }

    // Engine to ui controls
    if constexpr(control_inputs_type::size > 0 || control_outputs_type::size > 0)
    {
      // Update the value in the UI
      std::weak_ptr<safe_node<Node>> weak_node = ptr;
      ExecutorGuiUpdate<Node> timer_action{weak_node, element};
      timer_action();

      con(
          ctx.doc.coarseUpdateTimer, &QTimer::timeout, this, [=] { timer_action(); },
          Qt::QueuedConnection);
    }
  }

  void connect_message_bus(
      ProcessModel<Node>& element, const ::Execution::Context& ctx,
      std::shared_ptr<safe_node<Node>>& ptr)
  {
    // Custom UI messages to engine
    avnd::effect_container<Node>& eff = ptr->impl;
    if constexpr(avnd::has_gui_to_processor_bus<Node>)
    {
      element.from_ui = [p = QPointer{this}, &eff](QByteArray b) {
        if(!p)
          return;

        p->in_exec([mess = std::move(b), &eff] {
          using refl = avnd::function_reflection<&Node::process_message>;
          static_assert(refl::count <= 1);

          if constexpr(refl::count == 0)
          {
            // no arguments, just call it
            eff.effect.process_message();
          }
          else if constexpr(refl::count == 1)
          {
            using arg_type = avnd::first_argument<&Node::process_message>;
            std::decay_t<arg_type> arg;
            MessageBusReader b{mess};
            b(arg);
            eff.effect.process_message(std::move(arg));
          }
        });
      };
    }

    if constexpr(avnd::has_processor_to_gui_bus<Node>)
    {
      eff.effect.send_message = [this](auto b) mutable {
        this->in_edit([this, bb = std::move(b)]() mutable {
          MessageBusSender{this->process().to_ui}(std::move(bb));
        });
      };
    }
  }

  void connect_worker(
      ProcessModel<Node>& element, const ::Execution::Context& ctx,
      std::shared_ptr<safe_node<Node>>& ptr)
  {
    if constexpr(avnd::has_worker<Node>)
    {
      avnd::effect_container<Node>& eff = ptr->impl;
      // Initialize the thread pool beforehand
      auto& tq = score::TaskPool::instance();
      using worker_type = decltype(eff.effect.worker);
      for(auto& eff : eff.effects())
      {
        std::weak_ptr eff_ptr = std::shared_ptr<Node>(this->node, &eff);
        std::weak_ptr qex_ptr = std::shared_ptr<Execution::ExecutionCommandQueue>(
            ctx.alias.lock(), &ctx.executionQueue);

        eff.worker.request
            = [&tq, qex_ptr = std::move(qex_ptr),
               eff_ptr = std::move(eff_ptr)]<typename... Args>(Args&&... f) {
          // request() is invoked in the DSP / processor thread
          // and just posts the task to the thread pool
          tq.post([eff_ptr = std::move(eff_ptr), qex_ptr = std::move(qex_ptr),
                   ... ff = std::forward<Args>(f)]() mutable {
            // This happens in the worker thread
            // If for some reason the object has already been removed, not much
            // reason to perform the work
            if(!eff_ptr.lock())
              return;

            auto res = worker_type::work(std::forward<decltype(ff)>(ff)...);
            if(!res)
              return;

            // Execution queue is currently spsc from main thread to an exec thread,
            // we cannot just yeet the result back from the thread-pool
            ossia::qt::run_async(
                qApp, [eff_ptr = std::move(eff_ptr), qex_ptr = std::move(qex_ptr),
                       res = std::move(res)]() mutable {
                  // Main thread
                  std::shared_ptr qex = qex_ptr.lock();
                  if(!qex)
                    return;

                  qex->enqueue(
                      [eff_ptr = std::move(eff_ptr), res = std::move(res)]() mutable {
                // DSP / processor thread
                // We need res to be mutable so that the worker can use it to e.g. store
                // old data which will be freed back in the main thread
                if(auto p = eff_ptr.lock())
                  res(*p);
                  });
                });
          });
        };
      }
    }
  }

  // Update everything
  void update_controls(std::shared_ptr<safe_node<Node>>& ptr)
  {
    avnd::effect_container<Node>& eff = ptr->impl;
    {
      for(auto& state : eff.full_state())
      {
        avnd::input_introspection<Node>::for_all(
            state.inputs, [&](auto& field) { if_possible(field.update(state.effect)); });
      }
    }
  }

  void cleanup() override
  {
    if constexpr(requires { this->process().from_ui; })
    {
      this->process().from_ui = [](QByteArray arr) {};
    }
    // FIXME cleanup eff.effect.send_message too ?

#if SCORE_PLUGIN_GFX
    if constexpr(is_gpu<Node>)
    {
      // FIXME this must move in the Node dtor. See video_node
      auto& gfx_exec = this->system().doc.template plugin<Gfx::DocumentPlugin>().exec;
      if(node_id >= 0)
        gfx_exec.ui->unregister_node(node_id);
    }

    // FIXME refactor this with other GFX processes
    for(auto* outlet : this->process().outlets())
    {
      if(auto out = qobject_cast<Gfx::TextureOutlet*>(outlet))
      {
        out->nodeId = -1;
      }
    }
#endif
    ::Execution::ProcessComponent::cleanup();
  }

  ~Executor() { }
};
}
