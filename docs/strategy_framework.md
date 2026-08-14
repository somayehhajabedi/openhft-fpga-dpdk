                    MARKET DATA PATH
                    =================

NIC / PCAP / ITCH
       ↓
ITCH Parser
       ↓
MarketDataEvent
       ↓
┌─────────────────────┐
│     SPSC Queue      │
└─────────────────────┘
       ↓
MarketDataPipeline
       ↓
       ├──────────────→ MarketDataBookConsumer
       │                     ↓
       │                Market Order Book
       │
       └──────────────→ MarketDataStrategyConsumer
                              ↓
                        StrategyEngine
                              ↓
                           Strategy
                              ↓
                         OrderIntent


                    ORDER PATH
                    ==========

                         OrderIntent
                              ↓
                           Gateway
                              ↓
                        RiskManager
                              ↓
                     OrderExecutionSink
                              ↓
                MatchingEngineExecutionSink
                              ↓
                  MatchingEngine::submitOrder()
                              ↓
                          OrderPool
                              ↓
                           Order*
                              ↓
                          Matching