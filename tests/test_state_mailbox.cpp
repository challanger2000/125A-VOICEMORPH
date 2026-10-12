#include "ParameterStateMailbox.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <thread>
#include <iostream>

namespace {
// MSVC requires a namespace-scope integral constant for std::array's
// template size when used from a thread lambda.
constexpr std::size_t kCount=19;
}
int main() {
 vm::ParameterStateMailbox<kCount> mailbox;
 std::array<float,kCount> initial{},out{};
 for(std::size_t i=0;i<kCount;++i)initial[i]=static_cast<float>(i)*.01f;
 mailbox.initialize(initial.data());
 assert(!mailbox.consumeAudio(out.data()));
 mailbox.publish(initial.data());
 assert(mailbox.consumeAudio(out.data()));
 for(std::size_t i=0;i<kCount;++i)assert(out[i]==initial[i]);
 assert(!mailbox.consumeAudio(out.data()));
 // When several project states arrive before an audio block, only the
 // latest complete revision should be applied at that block boundary.
 std::array<float,kCount> intermediate{};
 for(std::size_t i=0;i<kCount;++i)intermediate[i]=2.f+static_cast<float>(i)*.01f;
 mailbox.publish(intermediate.data());
 for(std::size_t i=0;i<kCount;++i)intermediate[i]=3.f+static_cast<float>(i)*.01f;
 mailbox.publish(intermediate.data());
 assert(mailbox.consumeAudio(out.data()));
 for(std::size_t i=0;i<kCount;++i)assert(std::fabs(out[i]-intermediate[i])<1.e-6f);
 assert(!mailbox.consumeAudio(out.data()));
 // As soon as host restores a new project, it must arrive as a single
 // complete 19-parameter vector, not partial offsets from another revision.
 std::atomic<bool> done{false};
 std::atomic<int> complete{0};
 std::thread host([&]{
  for(int iteration=1;iteration<=20000;++iteration){
   std::array<float,kCount> state{};
   for(std::size_t i=0;i<kCount;++i)
    state[i]=static_cast<float>(iteration)+static_cast<float>(i)*.01f;
   mailbox.publish(state.data());
  }
  done.store(true,std::memory_order_release);
 });
 while(!done.load(std::memory_order_acquire)){
  if(!mailbox.consumeAudio(out.data()))continue;
  for(std::size_t i=1;i<kCount;++i){
   assert(std::fabs((out[i]-out[0])-static_cast<float>(i)*.01f)<.004f);
  }
  complete.fetch_add(1,std::memory_order_relaxed);
 }
 host.join();
 // After racing, the final state must arrive unmodified.
 assert(mailbox.consumeAudio(out.data())||out[0]==20000.f);
 assert(out[0]==20000.f);
 mailbox.storeAutomationAudio(5,.8f);
 mailbox.snapshot(out.data());
 assert(out[5]==.8f);
 assert(out[0]==20000.f);
 std::cout<<"State mailbox concurrency and automation PASS, snapshots="
          <<complete.load()<<"\n";
}
