#include "MetroModel.hpp"

#include <Process/Dataflow/Port.hpp>
#include <wobjectimpl.h>
W_OBJECT_IMPL(Media::Metro::Model)

namespace Media::Metro
{

Model::Model(
    const TimeVal& duration,
    const Id<Process::ProcessModel>& id,
    QObject* parent)
    : Process::ProcessModel{duration,
                            id,
                            Metadata<ObjectKey_k, ProcessModel>::get(),
                            parent}
    , audio_outlet{Process::make_audio_outlet(Id<Process::Port>(0), this)}
    , bang_outlet{Process::make_value_outlet(Id<Process::Port>(1), this)}
{
  audio_outlet->setPropagate(true);

  bang_outlet->type = Process::PortType::Message;

  metadata().setInstanceName(*this);
  init();
}

Model::~Model() {}

void Model::init() {
  m_outlets.push_back(audio_outlet.get());
  m_outlets.push_back(bang_outlet.get());
}

}

template <>
void DataStreamReader::read(const Media::Metro::Model& proc)
{
  m_stream << *proc.audio_outlet << *proc.bang_outlet;
  insertDelimiter();
}

template <>
void DataStreamWriter::write(Media::Metro::Model& proc)
{
  proc.audio_outlet = Process::load_audio_outlet(*this, &proc);
  proc.bang_outlet = Process::load_value_outlet(*this, &proc);
  checkDelimiter();
}

template <>
void JSONObjectReader::read(const Media::Metro::Model& proc)
{
  obj["Outlet"] = toJsonObject(*proc.audio_outlet);
  obj["BangOutlet"] = toJsonObject(*proc.bang_outlet);
}

template <>
void JSONObjectWriter::write(Media::Metro::Model& proc)
{
  {
    JSONObjectWriter writer{obj["Outlet"].toObject()};
    proc.audio_outlet = Process::load_audio_outlet(writer, &proc);
  }
  {
    JSONObjectWriter writer{obj["BangOutlet"].toObject()};
    proc.bang_outlet = Process::load_value_outlet(writer, &proc);
  }
}
