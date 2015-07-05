#include "DisplayedStateModel.hpp"

#include "StateView.hpp"

#include "Document/Constraint/ViewModels/AbstractConstraintView.hpp"
#include "Document/Event/EventModel.hpp"

#include "Process/ScenarioModel.hpp"
#include "Process/Temporal/TemporalScenarioPresenter.hpp"

DisplayedStateModel::DisplayedStateModel(const id_type<DisplayedStateModel>& id,
                                         const id_type<EventModel>& eventId,
                                         double yPos,
                                         QObject *parent):
    IdentifiedObject<DisplayedStateModel> {id, "DisplayedStateModel", parent},
    m_eventId{eventId},
    m_heightPercentage{yPos}
{

}

DisplayedStateModel::DisplayedStateModel(const DisplayedStateModel &source,
                                         const id_type<DisplayedStateModel> &id,
                                         QObject *parent):
    DisplayedStateModel{id, source.eventId(), source.heightPercentage(), parent}
{
    m_states = source.states();
}

const ScenarioModel* DisplayedStateModel::parentScenario() const
{
    return (dynamic_cast<ScenarioModel*>(parent()));
}

double DisplayedStateModel::heightPercentage() const
{
    return m_heightPercentage;
}

void DisplayedStateModel::setHeightPercentage(double y)
{
    if(m_heightPercentage == y)
        return;
    m_heightPercentage = y;
}

const id_type<EventModel> &DisplayedStateModel::eventId() const
{
    return m_eventId;
}

void DisplayedStateModel::setNextConstraint(const id_type<ConstraintModel> & id)
{
    m_nextConstraint = id;
}

void DisplayedStateModel::setPreviousConstraint(const id_type<ConstraintModel> & id)
{
    m_previousConstraint = id;
}


const iscore::StateList& DisplayedStateModel::states() const
{
    return m_states;
}

void DisplayedStateModel::replaceStates(const iscore::StateList &newStates)
{
    m_states = newStates;
    emit statesChanged();
}

void DisplayedStateModel::addState(const iscore::State &s)
{
    m_states.append(s);
    emit statesChanged();
}

void DisplayedStateModel::removeState(const iscore::State &s)
{
    m_states.removeOne(s);
    emit statesChanged();
}

