#include "BaseElementModel.hpp"

#include "source/Document/Constraint/ConstraintModel.hpp"
#include "source/Document/Constraint/ViewModels/FullView/FullViewConstraintViewModel.hpp"

#include <iscore/document/DocumentInterface.hpp>

#include "Commands/Constraint/AddProcessToConstraint.hpp"
#include "Commands/Constraint/AddBoxToConstraint.hpp"
#include "Commands/Constraint/Box/AddDeckToBox.hpp"
#include "Commands/Constraint/Box/Deck/ResizeDeckVertically.hpp"
#include "Commands/Constraint/Box/Deck/AddProcessViewModelToDeck.hpp"
#include "Commands/Scenario/ShowBoxInViewModel.hpp"

#include "Document/Constraint/Box/BoxModel.hpp"
#include "Document/Constraint/Box/Deck/DeckModel.hpp"
#include "ProcessInterface/ProcessSharedModelInterface.hpp"
#include "ProcessInterface/ProcessViewModelInterface.hpp"

using namespace Scenario;

BaseElementModel::BaseElementModel(QObject* parent) :
    iscore::DocumentDelegateModelInterface {id_type<iscore::DocumentDelegateModelInterface>(getNextId()), "BaseElementModel", parent},
    m_baseConstraint {new ConstraintModel{
                            id_type<ConstraintModel>{0},
                            id_type<AbstractConstraintViewModel>{0},
                            0,
                            this}}
{
    ConstraintModel::Algorithms::changeAllDurations(*m_baseConstraint, std::chrono::minutes{3});
    m_baseConstraint->setObjectName("BaseConstraintModel");

    initializeNewDocument(m_baseConstraint->fullView());
    setFocusedViewModel(m_baseConstraint->boxes().front()->decks().front()->processViewModels().front());
}

void BaseElementModel::initializeNewDocument(const FullViewConstraintViewModel *viewmodel)
{
    using namespace Scenario::Command;
    auto constraint_model = viewmodel->model();

    AddProcessToConstraint cmd1
    {
        {
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}}
        },
        "Scenario"
    };
    cmd1.redo();
    auto scenarioId = constraint_model->processes().front()->id();

    AddBoxToConstraint cmd2
    {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}}
        }
    };
    cmd2.redo();
    auto box = constraint_model->boxes().front();

    ShowBoxInViewModel cmd3 {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"FullViewConstraintViewModel", viewmodel->id()}
        },
        box->id() };
    cmd3.redo();

    AddDeckToBox cmd4
    {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"BoxModel", box->id() }
        }
    };
    cmd4.redo();
    auto deckId = box->decks().front()->id();

    ResizeDeckVertically cmd5
    {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"BoxModel", box->id() },
            {"DeckModel", deckId}
        },
        1500
    };
    cmd5.redo();

    AddProcessViewModelToDeck cmd6
    {
        {
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"BoxModel", box->id() },
            {"DeckModel", deckId}
        },
        {
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"ScenarioModel", scenarioId}
        }
    };
    cmd6.redo();
}


void BaseElementModel::setFocusedViewModel(ProcessViewModelInterface* proc)
{
    if(proc != m_focusedViewModel)
    {
        m_focusedViewModel = proc;
        emit focusedViewModelChanged();
    }
}



void BaseElementModel::setNewSelection(const Selection& s)
{
    if(s.empty())
    {
        if(m_focusedProcess)
        {
            m_focusedProcess->setSelection({});
            m_displayedConstraint->selection.set(false);
            m_focusedProcess = nullptr;
        }
    }
    else if(s.first() == m_displayedConstraint)
    {
        m_displayedConstraint->selection.set(true);
    }
    else
    {
        // We know by the presenter that all objects
        // in a given selection are in the same Process.
        m_displayedConstraint->selection.set(false);
        auto newProc = parentProcess(s.first());
        if(m_focusedProcess && newProc != m_focusedProcess)
        {
            m_focusedProcess->setSelection({});
        }

        m_focusedProcess = newProc;
        m_focusedProcess->setSelection(s);
    }

    emit focusMe();
}

void BaseElementModel::setDisplayedConstraint(ConstraintModel *constraint)
{
    m_displayedConstraint = constraint;
}
