#pragma once
#include <QJsonObject>
#include <QStringList>
#include <QMap>


namespace Library
{
enum class Category
{
    StateNode, MessageList, ScenarioData, Process, Device, Address
};
const QMap<Category, QString>& categoryPrettyName();


struct LibraryElement
{
        QString name;
        Category category;
        QStringList tags;
        QJsonObject obj;
};
}
