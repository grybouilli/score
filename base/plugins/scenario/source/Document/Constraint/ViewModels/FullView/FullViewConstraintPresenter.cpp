#include "FullViewConstraintPresenter.hpp"

#include "Document/Event/EventModel.hpp"
#include "Document/Constraint/ConstraintModel.hpp"
#include "Document/Constraint/ViewModels/FullView/FullViewConstraintViewModel.hpp"
#include "Document/Constraint/ViewModels/FullView/FullViewConstraintView.hpp"
#include "Commands/Constraint/AddProcessToConstraint.hpp"

#include <iscore/document/DocumentInterface.hpp>
#include <core/document/Document.hpp>

#include <QGraphicsScene>

FullViewConstraintPresenter::FullViewConstraintPresenter(FullViewConstraintViewModel* cstr_model,
                                                         QGraphicsObject *parentobject,
                                                         QObject* parent) :
    AbstractConstraintPresenter {"FullViewConstraintPresenter",
                                 cstr_model,
                                 new FullViewConstraintView{*this, parentobject},
                                 parent},
    m_selectionDispatcher{iscore::IDocument::documentFromObject(cstr_model->model())->selectionStack()}
{
    if(viewModel(this)->isBoxShown())
    {
        on_boxShown(viewModel(this)->shownBox());
    }

    updateHeight();

    connect(::view(this), &FullViewConstraintView::constraintPressed,
            this,      &FullViewConstraintPresenter::on_pressed);
}

FullViewConstraintPresenter::~FullViewConstraintPresenter()
{
    if(::view(this))
    {
        auto sc = ::view(this)->scene();

        if(sc && sc->items().contains(::view(this)))
        {
            sc->removeItem(::view(this));
        }

        ::view(this)->deleteLater();
    }
}

void FullViewConstraintPresenter::on_pressed()
{
    m_selectionDispatcher.setAndCommit({this->model()});
}
