#pragma once
namespace photoboss
{
	struct ImageGroup;
	struct ThumbnailResult;
	class Pipeline;

	class IUiUpdateSink
	{
	public:
		virtual ~IUiUpdateSink() = default;
		virtual void addPendingGroup(const ImageGroup& group) = 0;
		virtual void updateGroup(const ImageGroup& group) = 0;
		virtual void setThumbnail(const ThumbnailResult& result) = 0;
		virtual void setPhaseProgress(Pipeline::Phase phase, int current, int total) = 0;
		// Optionally, a method for status messages
		virtual void setStatusMessage(const QString& message) = 0;
		virtual void setPipelineState(Pipeline::PipelineState state) = 0;
	};
}