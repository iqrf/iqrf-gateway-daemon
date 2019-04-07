#pragma once

#include <memory>
#include "PreparedData.h"
#include "IOtaUploadService.h"

namespace iqrf {

  /// \class OtaUploadService
  /// \brief Prepares data from code file
  class DataPreparer
  {
  public:

    /// \brief Processes specified file with specified content type and returns prepared data.
    /// \param loadingContent		type of content of specified source file
    /// \param fileName					source file
    /// \param isForBroadcast		indicates, whether the data shall be prepared for broadcast
    /// \return									data corresponding to content of specified source file
    static std::unique_ptr<PreparedData> prepareData(
      IOtaUploadService::LoadingContentType loadingContent,
      const std::string& fileName,
      bool isForBroadcast
    );

  private:
    class Imp;
    static Imp* m_imp;
  };
}