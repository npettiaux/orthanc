/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "../PrecompiledHeadersServer.h"
#include "OrthancRestApi.h"

#include "../OrthancInitialization.h"
#include "../FromDcmtkBridge.h"
#include "../../Plugins/Engine/PluginsManager.h"
#include "../../Plugins/Engine/OrthancPlugins.h"

#include <glog/logging.h>


namespace Orthanc
{
  // System information -------------------------------------------------------

  static void ServeRoot(RestApiGetCall& call)
  {
    call.GetOutput().Redirect("app/explorer.html");
  }
 
  static void GetSystemInformation(RestApiGetCall& call)
  {
    Json::Value result = Json::objectValue;

    result["Version"] = ORTHANC_VERSION;
    result["Name"] = Configuration::GetGlobalStringParameter("Name", "");
    result["DatabaseVersion"] = boost::lexical_cast<int>(OrthancRestApi::GetIndex(call).GetGlobalProperty(GlobalProperty_DatabaseSchemaVersion, "0"));

    call.GetOutput().AnswerJson(result);
  }

  static void GetStatistics(RestApiGetCall& call)
  {
    Json::Value result = Json::objectValue;
    OrthancRestApi::GetIndex(call).ComputeStatistics(result);
    call.GetOutput().AnswerJson(result);
  }

  static void GenerateUid(RestApiGetCall& call)
  {
    std::string level = call.GetArgument("level", "");
    if (level == "patient")
    {
      call.GetOutput().AnswerBuffer(FromDcmtkBridge::GenerateUniqueIdentifier(ResourceType_Patient), "text/plain");
    }
    else if (level == "study")
    {
      call.GetOutput().AnswerBuffer(FromDcmtkBridge::GenerateUniqueIdentifier(ResourceType_Study), "text/plain");
    }
    else if (level == "series")
    {
      call.GetOutput().AnswerBuffer(FromDcmtkBridge::GenerateUniqueIdentifier(ResourceType_Series), "text/plain");
    }
    else if (level == "instance")
    {
      call.GetOutput().AnswerBuffer(FromDcmtkBridge::GenerateUniqueIdentifier(ResourceType_Instance), "text/plain");
    }
  }

  static void ExecuteScript(RestApiPostCall& call)
  {
    std::string result;
    ServerContext& context = OrthancRestApi::GetContext(call);

    {
      ServerContext::LuaContextLocker locker(context);
      locker.GetLua().Execute(result, call.GetPostBody());
    }

    call.GetOutput().AnswerBuffer(result, "text/plain");
  }

  static void GetNowIsoString(RestApiGetCall& call)
  {
    call.GetOutput().AnswerBuffer(Toolbox::GetNowIsoString(), "text/plain");
  }


  static void GetDicomConformanceStatement(RestApiGetCall& call)
  {
    std::string statement;
    GetFileResource(statement, EmbeddedResources::DICOM_CONFORMANCE_STATEMENT);
    call.GetOutput().AnswerBuffer(statement, "text/plain");
  }


  // Plugins information ------------------------------------------------------

  static void ListPlugins(RestApiGetCall& call)
  {
    Json::Value v = Json::arrayValue;

    v.append("explorer.js");

    if (OrthancRestApi::GetContext(call).HasPlugins())
    {
      std::list<std::string> plugins;
      OrthancRestApi::GetContext(call).GetPluginsManager().ListPlugins(plugins);

      for (std::list<std::string>::const_iterator 
             it = plugins.begin(); it != plugins.end(); ++it)
      {
        v.append(*it);
      }
    }

    call.GetOutput().AnswerJson(v);
  }


  static void GetPlugin(RestApiGetCall& call)
  {
    if (!OrthancRestApi::GetContext(call).HasPlugins())
    {
      return;
    }

    const PluginsManager& manager = OrthancRestApi::GetContext(call).GetPluginsManager();
    std::string id = call.GetUriComponent("id", "");

    if (manager.HasPlugin(id))
    {
      Json::Value v = Json::objectValue;
      v["ID"] = id;
      v["Version"] = manager.GetPluginVersion(id);

      const OrthancPlugins& plugins = OrthancRestApi::GetContext(call).GetOrthancPlugins();
      const char *c = plugins.GetProperty(id.c_str(), _OrthancPluginProperty_RootUri);
      if (c != NULL)
      {
        v["RootUri"] = c;
      }

      c = plugins.GetProperty(id.c_str(), _OrthancPluginProperty_Description);
      if (c != NULL)
      {
        v["Description"] = c;
      }

      c = plugins.GetProperty(id.c_str(), _OrthancPluginProperty_OrthancExplorer);
      v["ExtendsOrthancExplorer"] = (c != NULL);

      call.GetOutput().AnswerJson(v);
    }
  }


  static void GetOrthancExplorerPlugins(RestApiGetCall& call)
  {
    std::string s = "// Extensions to Orthanc Explorer by the registered plugins\n\n";

    if (OrthancRestApi::GetContext(call).HasPlugins())
    {
      const PluginsManager& manager = OrthancRestApi::GetContext(call).GetPluginsManager();
      const OrthancPlugins& plugins = OrthancRestApi::GetContext(call).GetOrthancPlugins();

      std::list<std::string> lst;
      OrthancRestApi::GetContext(call).GetPluginsManager().ListPlugins(lst);

      for (std::list<std::string>::const_iterator
             it = lst.begin(); it != lst.end(); ++it)
      {
        const char* tmp = plugins.GetProperty(it->c_str(), _OrthancPluginProperty_OrthancExplorer);
        if (tmp != NULL)
        {
          s += "/**\n * From plugin: " + *it + " (version " + manager.GetPluginVersion(*it) + ")\n **/\n\n";
          s += std::string(tmp) + "\n\n";
        }
      }
    }

    call.GetOutput().AnswerBuffer(s, "application/javascript");
  }


  void OrthancRestApi::RegisterSystem()
  {
    Register("/", ServeRoot);
    Register("/system", GetSystemInformation);
    Register("/statistics", GetStatistics);
    Register("/tools/generate-uid", GenerateUid);
    Register("/tools/execute-script", ExecuteScript);
    Register("/tools/now", GetNowIsoString);
    Register("/tools/dicom-conformance", GetDicomConformanceStatement);

    Register("/plugins", ListPlugins);
    Register("/plugins/{id}", GetPlugin);
    Register("/plugins/explorer.js", GetOrthancExplorerPlugins);
  }
}
