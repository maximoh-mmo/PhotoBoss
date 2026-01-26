#pragma once
namespace photoboss {

    /// <summary>
    /// 
	/// A pipeline stage that routes items from a single input queue to multiple output queues
	/// Takes a threadsafe input queue and a vector of threadsafe output queues.
	/// Run function reads from input queue, applies routing function to determine output queue index,
    /// 
	/// Route function should return an integer index corresponding to the output queue.
    /// 
    /// </summary>
    /// <typeparam name="In"></typeparam>
    /// <typeparam name="Out"></typeparam>
    template<typename In, typename Out>
    class RouterStage : public PipelineStage
    {
    public:
        explicit RouterStage(Queue<In>& input, const std::vector<Queue<Out>*>& outputs,
            std::function<int(const In&)> routeFunc)
            : m_input(input), m_outputs(outputs), m_route(routeFunc) {
        }

        void Run() override {
            In item;
            while (m_input.wait_and_pop(item)) {
                int idx = m_route(item);
                if (idx >= 0 && idx < static_cast<int>(m_outputs.size())) {
                    m_outputs[idx]->emplace(item);
                }
            }
            // Shut down all output queues
            for (auto* q : m_outputs) {
                q->shutdown();
            }
        }

    protected:
        Queue<In>& m_input;
        std::vector<Queue<Out>*> m_outputs;
        std::function<int(const In&)> m_route;
    };
}
