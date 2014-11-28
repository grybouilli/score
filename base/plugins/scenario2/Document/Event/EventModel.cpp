#include "EventModel.hpp"
#include <API/Headers/Editor/TimeNode.h>
#include <QVector>

EventModel::EventModel(int id, QObject* parent):
	QIdentifiedObject{parent, "EventModel", id},
	m_timeNode{new OSSIA::TimeNode}
{
	// TODO : connect to the timenode handlers so that the links to the intervals are correctly created.

}

EventModel::EventModel(QDataStream& s, QObject* parent):
	QIdentifiedObject{nullptr, "EventModel", -1}
{
	qDebug(Q_FUNC_INFO);
	int id;
	s >> id >> m_previousIntervals >> m_nextIntervals;
	this->setId(id);

	m_timeNode = new OSSIA::TimeNode;
	// TODO load
	this->setParent(parent); // Safer, if somebody receives a signal

}

QDataStream& operator << (QDataStream& s, const EventModel& ev)
{
	qDebug(Q_FUNC_INFO);
	s << ev.id()
	  << ev.m_previousIntervals
	  << ev.m_nextIntervals;

	// TODO s << ev.m_timeNode->save();
}
