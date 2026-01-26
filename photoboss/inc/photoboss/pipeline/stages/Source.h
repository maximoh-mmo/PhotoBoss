#pragma once
#include "Pipeline.h"

namespace photoboss
{
	/// <summary>
	///	
	/// Stage that produces data into the pipeline.
	/// 
	/// </summary>
	/// <typeparam name="Out"></typeparam>

	template<typename Out>
	class Source : public PipelineStage
	{
	public:
		explicit Source(Queue<Out>& output)
			: m_output(output)
		{
		}

		virtual void produce() = 0;

		void run() override
		{
			produce();
			m_output.shutdown();
		}

		virtual ~Source() = default;
	protected:
		Queue<Out>& m_output;
	};
}