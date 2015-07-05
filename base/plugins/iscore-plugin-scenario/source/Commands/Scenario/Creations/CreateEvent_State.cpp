#include "CreateEvent_State.hpp"

#include "Process/Algorithms/StandardCreationPolicy.hpp"
#include "Process/ScenarioModel.hpp"

#include <iscore/tools/SettableIdentifierGeneration.hpp>

using namespace Scenario::Command;
CreateEvent_State::CreateEvent_State(
        const ScenarioModel& scenario,
        const id_type<TimeNodeModel>& timeNode,
        double stateY):
    iscore::SerializableCommand{"ScenarioControl", commandName(), description()},
    m_newEvent{getStrongId(scenario.events())},
    m_command{scenario,
              m_newEvent,
              stateY},
    m_timeNode{timeNode}
{

}

void CreateEvent_State::undo()
{
    m_command.undo();

    ScenarioCreate<EventModel>::undo(
                m_newEvent,
                m_command.scenarioPath().find<ScenarioModel>());
}

void CreateEvent_State::redo()
{
    auto& scenar = m_command.scenarioPath().find<ScenarioModel>();

    // Create the event
    ScenarioCreate<EventModel>::redo(
                m_newEvent,
                scenar.timeNode(m_timeNode),
                {m_command.endStateY() - 0.1, m_command.endStateY() + 0.1},
                scenar);

    // And the state
    m_command.redo();
}

void CreateEvent_State::serializeImpl(QDataStream& s) const
{
    s << m_newEvent << m_command.serialize() << m_timeNode;
}

void CreateEvent_State::deserializeImpl(QDataStream& s)
{
    QByteArray b;
    s >> m_newEvent >> b >> m_timeNode;

    m_command.deserialize(b);
}
