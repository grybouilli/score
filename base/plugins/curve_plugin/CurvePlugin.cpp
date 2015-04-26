#include "CurvePlugin.hpp"

#include "Inspector/AutomationInspectorFactory.hpp"
#include "Inspector/AutomationStateInspectorFactory.hpp"
#include "Automation/AutomationFactory.hpp"
#include "AutomationControl.hpp"

CurvePlugin::CurvePlugin() :
    QObject {}
{
    setObjectName("CurvePlugin");
}

iscore::PluginControlInterface* CurvePlugin::control_make()
{
    return new AutomationControl{nullptr};
}

QVector<iscore::FactoryInterface*> CurvePlugin::factories_make(QString factoryName)
{
    if(factoryName == "Process")
    {
        return {new AutomationFactory};
    }

    if(factoryName == "Inspector")
    {
        return {new AutomationInspectorFactory,
                new AutomationStateInspectorFactory};
    }

    return {};
}
