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
		virtual void incrementPhaseProgress(Pipeline::Phase phase, int increment) = 0;
		virtual void setFileTotal(int total) = 0;
		virtual void setStatusMessage(const QString& message) = 0;
		virtual void setPipelineState(Pipeline::PipelineState state) = 0;
	};
}