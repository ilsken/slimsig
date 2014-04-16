#include <iostream>
#include <bandit/bandit.h>
#include <slimsig/slimsig.h>

using namespace bandit;
namespace ss = slimsig;
using ss::connection;
using ss::scoped_connection;
using slimsig::make_scoped_connection;

bool function_slot_triggered = false;
bool static_slot_triggered = false;
bool functor_slot_triggered = false;
void function_slot() { function_slot_triggered = true; }
struct class_test {
  static void static_slot() { static_slot_triggered = true; }
  bool bound_slot_triggered = false;
  void bound_slot() { bound_slot_triggered = true; }
  void operator() () { functor_slot_triggered = true; }
};

go_bandit([]
{
  describe("signal", []
  {
    ss::signal<void()> signal;
    before_each([&]
    {
      signal = ss::signal<void()>{};
    });
    
    it("should trigger basic function slots", [&]
    {
      signal.connect(&function_slot);
      signal.emit();
      AssertThat(function_slot_triggered, Equals(true));
    });
    
    it("should trigger static method slots", [&]
    {
      signal.connect(&class_test::static_slot);
      signal.emit();
      AssertThat(static_slot_triggered, Equals(true));
    });
    it("should trigger bound member function slots", [&]
    {
      class_test obj;
      signal.connect(std::bind(&class_test::bound_slot, &obj));
      signal.emit();
      AssertThat(obj.bound_slot_triggered, Equals(true));
    });
    it("should trigger functor slots", [&]
    {
      class_test obj;
      signal.connect(obj);
      signal.emit();
      AssertThat(functor_slot_triggered, Equals(true));
    });
    describe("#slot_count()", [&] {
      it("should return the slot count", [&]
      {
        signal.connect([]{});
        AssertThat(signal.slot_count(), Equals(1));
      });
      it("should return the correct count when adding slots during iteration", [&]
      {
        signal.connect([&] {
          signal.connect([]{});
          AssertThat(signal.slot_count(), Equals(2));
        });
        signal.emit();
        AssertThat(signal.slot_count(), Equals(2));
      });
    });
    describe("#disconnect_all()", [&]
    {
      it("should remove all slots", [&]
      {
        auto conn1 = signal.connect([]{});
        auto conn2 = signal.connect([]{});
        signal.disconnect_all();
        AssertThat(signal.slot_count(), Equals(0));
        AssertThat(conn1.connected(), Equals(false));
        AssertThat(conn2.connected(), Equals(false));
        AssertThat(signal.empty(), Equals(true));
      });
    });
    
  });
  describe("connection", [] {
    ss::signal<void()> signal;
    before_each([&] { signal = ss::signal<void()>{}; });
    describe("#connected()", [&]
    {
      it("should return whether or not the slot is connected", [&]
      {
        auto connection = signal.connect([]{});
        AssertThat(connection.connected(), Equals(true));
        signal.disconnect_all();
        AssertThat(connection.connected(), Equals(false));
      });
    });
    describe("#disconnect", [&]{
      it("should disconnect the slot",[&]
      {
        bool fired = false;
        auto connection = signal.connect([&] { fired = true; });
        connection.disconnect();
        signal.emit();
        AssertThat(fired, Equals(false));
        AssertThat(connection.connected(), Equals(false));
      });
      it("should not throw if already disconnected", [&]
      {
        auto connection = signal.connect([]{});
        connection.disconnect();
        connection.disconnect();
      });
    });
    it("should be consistent across copies", [&]
    {
      auto conn1 = signal.connect([]{});
      auto conn2 = conn1;
      conn1.disconnect();
      AssertThat(conn1.connected(), Equals(conn2.connected()));
    });
    it("should not affect slot lifetime", [&]
    {
      bool fired = false;
      auto fn = [&] { fired = true; };
      {
        auto connection = signal.connect(fn);
      }
      signal.emit();
      AssertThat(fired, Equals(true));
    });
    it("should still be valid if the signal is destroyed", [&]
    {
      ss::connection<ss::signal<void()>> connection;
      {
        ss::signal<void()> scoped_signal;
        connection = scoped_signal.connect([]{});
      }
      AssertThat(connection.connected(), Equals(false));
    });
    
  });
  describe("scoped_connection", []{
    ss::signal<void()> signal;
    before_each([&] { signal = ss::signal<void()>(); });
    it("should disconnect the connection after leaving the scope", [&]
    {
      bool fired = false;
      {
        auto scoped = make_scoped_connection(signal.connect([&]{ fired = true ;}));
      }
      signal.emit();
      AssertThat(fired, Equals(false));
      AssertThat(signal.empty(), Equals(true));
    });
    it("should update state of underlying connection", [&]
    {
      auto connection = signal.connect([]{});
      {
        auto scoped = make_scoped_connection(connection);
      }
      signal.emit();
      AssertThat(connection.connected(), Equals(false));
    });
  });
});
int main(int argc, char* argv[]) {
  bandit::run(argc, argv);
}