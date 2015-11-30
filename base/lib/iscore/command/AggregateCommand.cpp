#include <boost/concept/usage.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <core/application/ApplicationComponents.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QtGlobal>
#include <QList>
#include <QPair>

#include "AggregateCommand.hpp"
#include <core/application/ApplicationContext.hpp>
#include <iscore/command/SerializableCommand.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include <iscore/serialization/DataStreamVisitor.hpp>

using namespace iscore;

AggregateCommand::~AggregateCommand()
{

}

void AggregateCommand::undo() const
{
    for (auto cmd : boost::adaptors::reverse(m_cmds))
    {
        cmd->undo();
    }
}

void AggregateCommand::redo() const
{
    for(auto cmd : m_cmds)
    {
        cmd->redo();
    }
}

void AggregateCommand::serializeImpl(DataStreamInput& s) const
{
    std::vector<CommandData> serializedCommands;
    serializedCommands.reserve(m_cmds.size());

    for(auto& cmd : m_cmds)
    {
        serializedCommands.emplace_back(*cmd);
    }

    Visitor<Reader<DataStream>> reader{s.stream().device()};
    readFrom_vector_obj_impl(reader, serializedCommands);
}

void AggregateCommand::deserializeImpl(DataStreamOutput& s)
{
    std::vector<CommandData> serializedCommands;
    Visitor<Writer<DataStream>> writer{s.stream().device()};
    writeTo_vector_obj_impl(writer, serializedCommands);

    for(const auto& cmd_pack : serializedCommands)
    {
        auto cmd = context.components.instantiateUndoCommand(cmd_pack);
        m_cmds.push_back(cmd);
    }
}
