#include <QApplication>
#include <QtDebug>
#include "pqPVApplicationCore.h"
#include "myMainWindow.h"
#include "pqBrandPluginsLoader.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);
  myMainWindow window;
  QString plugin_string = "VRPNPlugin";
  QStringList plugin_list = plugin_string.split(';',QString::SkipEmptyParts);
  pqBrandPluginsLoader loader;
  if (loader.loadPlugins(plugin_list) == false)
    {
    qCritical() << "Failed to load required plugins for this application";
    return false;
    }

  // Load optional plugins.
  plugin_string = "";
  plugin_list = plugin_string.split(';',QString::SkipEmptyParts);
  loader.loadPlugins(plugin_list, true); //quietly skip not-found plugins.



  window.show();
  return app.exec();
}
