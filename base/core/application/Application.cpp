#include <core/application/Application.hpp>

#include <core/model/Model.hpp>
#include <core/presenter/Presenter.hpp>
#include <core/view/View.hpp>
#include <core/application/ChildEventFilter.hpp>
using namespace iscore;

Application::Application(int& argc, char** argv):
	QObject{}
{
	// Application
	// Crashes if put in member initialization list... :(
	m_app = std::make_unique<QApplication>(argc, argv);
	this->setParent(m_app.get());
	this->setObjectName("Application");
	m_app->installEventFilter(new ChildEventFilter(this));

	QCoreApplication::setOrganizationName("OSSIA");
	QCoreApplication::setOrganizationDomain("i-score.com");
	QCoreApplication::setApplicationName("i-score");

	// Settings
	m_settings = std::make_unique<Settings>(this);

	// MVP
	m_model = new Model{this}; // ? Utile ? Ce sont les settings, non ?

	m_view = new View(qobject_cast<QObject*>(this));
	m_presenter = new Presenter(m_model, m_view, this);
	m_view->setPresenter(m_presenter);

	// Plugins
	m_pluginManager.reloadPlugins();
	loadPluginData();

	m_presenter->newDocument();

	// View
	m_view->show();
}

Application::~Application()
{
	this->setParent(nullptr);
}

void Application::loadPluginData()
{
	for(auto& set : m_pluginManager.m_settingsList)
		m_settings->setupSettingsPlugin(set);

	for(auto& cmd : m_pluginManager.m_commandList)
		m_presenter->setupCommand(cmd);

	for(auto& pnl : m_pluginManager.m_panelList)
		m_presenter->addPanel(pnl);
}

void Application::doConnections()
{
	// Trouver toutes les connections qui peuvent concerner ce "child"
	for(auto& a : m_pluginManager.m_autoconnections)
	{
		auto potential_sources = a.getMatchingChildrenForSource(this);
		auto potential_targets = a.getMatchingChildrenForTarget(this);
		potential_sources.append(a.getMatchingChildrenForSource(view()));
		potential_targets.append(a.getMatchingChildrenForTarget(view()));

		for(auto& s_elt : potential_sources)
		{
			for(auto& t_elt : potential_targets)
			{
				//s_elt->disconnect(a.source.method, t_elt);
				bool res = t_elt->connect(s_elt,
							   a.source.method,
							   a.target.method,
							   Qt::UniqueConnection);
			}
		}
	}
}

void Application::addAutoconnection(Autoconnect a)
{
	m_pluginManager.m_autoconnections.push_back(a);
	doConnections();
}

