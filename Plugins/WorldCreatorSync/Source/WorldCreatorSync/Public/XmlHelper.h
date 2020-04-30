#pragma once

#include "Runtime/XmlParser/Public/XmlParser.h"

class XmlHelper
{
public:
  static float GetFloat(FXmlNode* node, FString attributeName)
  {    
    auto att = node->GetAttribute(attributeName);
    if (att.IsEmpty())
      return 0.0f;
    return FCString::Atof(att.GetCharArray().GetData());
  }

  static int GetInt(FXmlNode* node, FString attributeName)
  {
    auto att = node->GetAttribute(attributeName);
    if (att.IsEmpty())
      return 0;
    return FCString::Atoi(att.GetCharArray().GetData());
  }

  static FString GetString(FXmlNode* node, FString attributeName)
  {
    auto att = node->GetAttribute(attributeName);
    if (att.IsEmpty())
      return TEXT("");
    return att;
  }
};