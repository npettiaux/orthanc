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


#include "../PrecompiledHeaders.h"
#include "RestApiOutput.h"

#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

#include "../OrthancException.h"

namespace Orthanc
{
  RestApiOutput::RestApiOutput(HttpOutput& output) : 
    output_(output),
    convertJsonToXml_(false)
  {
    alreadySent_ = false;
  }

  RestApiOutput::~RestApiOutput()
  {
  }

  void RestApiOutput::Finalize()
  {
    if (!alreadySent_)
    {
      output_.SendStatus(HttpStatus_404_NotFound);
    }
  }
  
  void RestApiOutput::CheckStatus()
  {
    if (alreadySent_)
    {
      throw OrthancException(ErrorCode_BadSequenceOfCalls);
    }
  }

  void RestApiOutput::AnswerFile(HttpFileSender& sender)
  {
    CheckStatus();
    sender.Send(output_);
    alreadySent_ = true;
  }

  void RestApiOutput::AnswerJson(const Json::Value& value)
  {
    CheckStatus();

    if (convertJsonToXml_)
    {
#if ORTHANC_PUGIXML_ENABLED == 1
      std::string s;
      Toolbox::JsonToXml(s, value);
      output_.SetContentType("application/xml");
      output_.SendBody(s);
#else
      LOG(ERROR) << "Orthanc was compiled without XML support";
      throw OrthancException(ErrorCode_InternalError);
#endif
    }
    else
    {
      Json::StyledWriter writer;
      output_.SetContentType("application/json");
      output_.SendBody(writer.write(value));
    }

    alreadySent_ = true;
  }

  void RestApiOutput::AnswerBuffer(const std::string& buffer,
                                   const std::string& contentType)
  {
    CheckStatus();
    output_.SetContentType(contentType.c_str());
    output_.SendBody(buffer);
    alreadySent_ = true;
  }

  void RestApiOutput::AnswerBuffer(const void* buffer,
                                   size_t length,
                                   const std::string& contentType)
  {
    CheckStatus();
    output_.SetContentType(contentType.c_str());
    output_.SendBody(buffer, length);
    alreadySent_ = true;
  }

  void RestApiOutput::Redirect(const std::string& path)
  {
    CheckStatus();
    output_.Redirect(path);
    alreadySent_ = true;
  }

  void RestApiOutput::SignalError(HttpStatus status)
  {
    if (status != HttpStatus_400_BadRequest &&
        status != HttpStatus_403_Forbidden &&
        status != HttpStatus_500_InternalServerError &&
        status != HttpStatus_415_UnsupportedMediaType)
    {
      throw OrthancException("This HTTP status is not allowed in a REST API");
    }

    CheckStatus();
    output_.SendStatus(status);
    alreadySent_ = true;    
  }

  void RestApiOutput::SetCookie(const std::string& name,
                                const std::string& value,
                                unsigned int maxAge)
  {
    if (name.find(";") != std::string::npos ||
        name.find(" ") != std::string::npos ||
        value.find(";") != std::string::npos ||
        value.find(" ") != std::string::npos)
    {
      throw OrthancException(ErrorCode_NotImplemented);
    }

    CheckStatus();

    std::string v = value + ";path=/";

    if (maxAge != 0)
    {
      v += ";max-age=" + boost::lexical_cast<std::string>(maxAge);
    }

    output_.SetCookie(name, v);
  }

  void RestApiOutput::ResetCookie(const std::string& name)
  {
    // This marks the cookie to be deleted by the browser in 1 second,
    // and before it actually gets deleted, its value is set to the
    // empty string
    SetCookie(name, "", 1);
  }
}
