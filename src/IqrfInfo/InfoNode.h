#pragma once

#include "EmbedNode.h"

namespace iqrf
{
  namespace embed
  {
    namespace node
    {
      namespace info
      {
        ////////////////
        class BriefInfo : public node::BriefInfo
        {
        public:
          BriefInfo(unsigned mid, bool disc, int hwpid, int hwpidVer, int osBuild, int dpaVer)
            :node::BriefInfo(mid, disc, hwpid, hwpidVer, osBuild, dpaVer)
          {}

          virtual ~BriefInfo() {}
        };
        typedef std::unique_ptr<BriefInfo> BriefInfoPtr;

      } //namespace info
    } //namespace node
  } //namespace embed
} //namespace iqrf
