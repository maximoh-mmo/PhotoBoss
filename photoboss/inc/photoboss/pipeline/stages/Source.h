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
	class Source : public StageBase
	{
	public:
		explicit Source(Queue<Out>& output, QString id, QObject* parent = nullptr) :
			StageBase(std::move(id), parent),
			m_output(output)
		{
		}

		virtual void produce() = 0;

		void run() override
		{
			produce();
		}

		virtual ~Source() = default;
	protected:
		Queue<Out>& m_output;

		// Inherited via StageBase
		void onStart() override
		{
			m_output.register_producer();
		}
		void onStop() override
		{
			m_output.producer_done();
		}
	};
}