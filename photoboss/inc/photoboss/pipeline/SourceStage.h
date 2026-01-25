#pragma once
#include "util/Queue.h"

namespace photoboss
{
	template<typename Out>
	class SourceStage : public PipelineStage
	{
	public:
		explicit SourceStage(Queue<Out>& output)
			: m_output(output)
		{
		}
		
		virtual void produce() = 0;
		
		void run() override
		{
			produce();
			m_output.shutdown();
		}

		virtual ~SourceStage() = default;
	protected:
		Queue<Out>& m_output;
	};
}