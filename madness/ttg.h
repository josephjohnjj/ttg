#ifndef MADNESS_TTG_H_INCLUDED
#define MADNESS_TTG_H_INCLUDED

#include "../ttg.h"

#include <array>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <madness/world/MADworld.h>
#include <madness/world/world_object.h>
#include <madness/world/worldhashmap.h>
#include <madness/world/worldtypes.h>

#include <madness/world/world_task_queue.h>

/*
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>

static inline pid_t gettid() {
    return syscall(SYS_gettid);
}
*/


namespace madness {
namespace ttg {



namespace detail {
World*& default_world_accessor() {
  static World* world_ptr = nullptr;
  return world_ptr;
}
}  // namespace detail

World& get_default_world() {
  if (detail::default_world_accessor() != nullptr) {
    return *detail::default_world_accessor();
  } else {
    throw "madness::ttg::set_default_world() must be called before use";
  }
}
void set_default_world(World& world) {
  detail::default_world_accessor() = &world;
}
void set_default_world(World* world) {
  detail::default_world_accessor() = world;
}

template <typename keyT, typename output_terminalsT, typename derivedT,
          typename... input_valueTs>
class Op : public ::ttg::OpBase,
           public WorldObject<
               Op<keyT, output_terminalsT, derivedT, input_valueTs...>> {
 private:
  World& world;
  std::shared_ptr<WorldDCPmapInterface<keyT>> pmap;

 protected:
  World& get_world() { return world; }
  std::shared_ptr<WorldDCPmapInterface<keyT>>& get_pmap() { return pmap; }

  using opT = Op<keyT, output_terminalsT, derivedT, input_valueTs...>;
  using worldobjT = WorldObject<opT>;

 public:
  static constexpr int numins =
      sizeof...(input_valueTs);  // number of input arguments
  static constexpr int numouts =
      std::tuple_size<output_terminalsT>::value;  // number of outputs or
                                                  // results

  using input_values_tuple_type = std::tuple<input_valueTs...>;
  using input_terminals_type = std::tuple<::ttg::In<keyT, input_valueTs>...>;
  using input_edges_type = std::tuple<::ttg::Edge<keyT, input_valueTs>...>;

  using output_terminals_type = output_terminalsT;
  using output_edges_type =
      typename ::ttg::terminals_to_edges<output_terminalsT>::type;

 private:
  input_terminals_type input_terminals;
  output_terminalsT output_terminals;

  struct OpArgs : TaskInterface {
    int counter;                      // Tracks the number of arguments set
    std::array<bool, numins> argset;  // Tracks if a given arg is already set;
    input_values_tuple_type t;        // The input values
    derivedT* derived;                // Pointer to derived class instance
    keyT key;                         // Task key

    OpArgs() : counter(numins), argset(), t() { std::fill(argset.begin(), argset.end(), false); }

    void run(World& world) {
        madness::print("starting task");
        derived->op(key, std::move(t), derived->output_terminals); // !!! NOTE moving t into op
        madness::print("finishing task");
    }

    virtual ~OpArgs() {}  // Will be deleted via TaskInterface*
  };

  using cacheT = ConcurrentHashMap<keyT, OpArgs*>;
  using accessorT = typename cacheT::accessor;
  cacheT cache;

  // Used to set the i'th argument (T is template to enable && collapsing)
  template <std::size_t i, typename T>
  //void set_arg(const keyT& key, const typename std::tuple_element<i, input_values_tuple_type>::type& value) {
  void set_arg(const keyT& key, T&& value) {

    using valueT = typename std::tuple_element<i, input_terminals_type>::type; // Should be same as T (or const T)

    ProcessID owner = pmap->owner(key);

    if (owner != world.rank()) {
      if (tracing())
          madness::print( world.rank(), ":", get_name(), " : ", key, ": forwarding setting argument : ", i);
      worldobjT::send(owner, &opT::template set_arg<i,const typename std::remove_reference<T>::type&>, key, value);
    } else {
      if (tracing())
          madness::print( world.rank(), ":", get_name(), " : ", key, ": setting argument : ", i);

      accessorT acc;
      if (cache.insert(acc, key))
        acc->second = new OpArgs();  // It will be deleted by the task q
      OpArgs* args = acc->second;

      if (args->argset[i]) {
        madness::print_error(world.rank(), ":", get_name(), " : ", key, ": error argument is already set : ", i);
        throw "bad set arg";
      }

      const char* isref[] = {" ","&"};
      std::cout << "about to assign arg " << isref[std::is_reference<T>::value] << demangled_type_name<T>() << "\n";
      
      std::get<i>(args->t) = std::forward<T>(value);
      args->argset[i] = true;
      args->counter--;
      if (args->counter == 0) {
        if (tracing())
            madness::print( world.rank(), ":", get_name(), " : ", key, ": submitting task for op ");
        args->derived = static_cast<derivedT*>(this);
        args->key = key;

        world.taskq.add(args);
        //static_cast<derivedT*>(this)->op(key, args->t, output_terminals); // Runs immediately
        
        cache.erase(key);
      }
    }
  }

  // Used to generate tasks with no input arguments
  void set_arg_empty(const keyT& key) {
    ProcessID owner = pmap->owner(key);

    if (owner != world.rank()) {
        if (tracing())
            madness::print(world.rank(), ":", get_name(), " : ", key, ": forwarding no-arg task: ");
        worldobjT::send(owner, &opT::set_arg_empty, key);
    } else {
        accessorT acc;
        if (cache.insert(acc, key))
            acc->second = new OpArgs();  // It will be deleted by the task q
        OpArgs* args = acc->second;
        
        if (tracing())
            madness::print(world.rank(), ":", get_name(), " : ", key, ": submitting task for op ");
        args->derived = static_cast<derivedT*>(this);
        args->key = key;
        
        world.taskq.add(args);
        //static_cast<derivedT*>(this)->op(key, args->t, output_terminals);// runs immediately
        
        cache.erase(key);
    }
  }

  // Used by invoke to set all arguments associated with a task
  template <size_t... IS>
  void set_args(std::index_sequence<IS...>, const keyT& key,
                const input_values_tuple_type& args) {
    int junk[] = {0, (set_arg<IS>(key, std::get<IS>(args)), 0)...};
    junk[0]++;
  }

  // Copy/assign/move forbidden ... we could make it work using
  // PIMPL for this base class.  However, this instance of the base
  // class is tied to a specific instance of a derived class a
  // pointer to which is captured for invoking derived class
  // functions.  Thus, not only does the derived class have to be
  // involved but we would have to do it in a thread safe way
  // including for possibly already running tasks and remote
  // references.  This is not worth the effort ... wherever you are
  // wanting to move/assign an Op you should be using a pointer.
  Op(const Op& other) = delete;
  Op& operator=(const Op& other) = delete;
  Op(Op&& other) = delete;
  Op& operator=(Op&& other) = delete;

  // Registers the callback for the i'th input terminal
  template <typename terminalT, std::size_t i>
  void register_input_callback(terminalT& input) {
    //using keyT = typename terminalT::key_type;
    using valueT = typename terminalT::value_type;      
    using move_callbackT = std::function<void(const keyT&, valueT&&)>;
    using send_callbackT = std::function<void(const keyT&, const valueT&)>;

    auto move_callback = [this](const keyT& key, valueT&& value) {
        std::cout << "move_callback\n";
        set_arg<i,valueT>(key, std::forward<typename terminalT::value_type>(value));
    };

    auto send_callback = [this](const keyT& key, const valueT& value) {
        std::cout << "send_callback\n";
        set_arg<i,const valueT&>(key, value);
    };
    
    input.set_callback(send_callbackT(send_callback), move_callbackT(move_callback));
  }

  template <std::size_t... IS>
  void register_input_callbacks(std::index_sequence<IS...>) {
    int junk[] = {
        0,
        (register_input_callback<
             typename std::tuple_element<IS, input_terminals_type>::type, IS>(
             std::get<IS>(input_terminals)),
         0)...};
    junk[0]++;
  }

  template <std::size_t... IS, typename inedgesT>
  void connect_my_inputs_to_incoming_edge_outputs(std::index_sequence<IS...>,
                                                  inedgesT& inedges) {
    int junk[] = {
        0,
        (std::get<IS>(inedges).set_out(&std::get<IS>(input_terminals)), 0)...};
    junk[0]++;
  }

  template <std::size_t... IS, typename outedgesT>
  void connect_my_outputs_to_outgoing_edge_inputs(std::index_sequence<IS...>,
                                                  outedgesT& outedges) {
    int junk[] = {
        0,
        (std::get<IS>(outedges).set_in(&std::get<IS>(output_terminals)), 0)...};
    junk[0]++;
  }

 public:
  Op(const std::string& name, const std::vector<std::string>& innames,
     const std::vector<std::string>& outnames,
     std::shared_ptr<WorldDCPmapInterface<keyT>> pmap = std::make_shared<WorldDCDefaultPmap<keyT>>(get_default_world()))
      : ::ttg::OpBase(name, numins, numouts),
        worldobjT(get_default_world()),
        world(get_default_world()),
        pmap(pmap) {
    // Cannot call these in base constructor since terminals not yet constructed
    if (innames.size() != std::tuple_size<input_terminals_type>::value)
      throw "madness::ttg::Op: #input names != #input terminals";
    if (outnames.size() != std::tuple_size<output_terminalsT>::value)
      throw "madness::ttg::Op: #output names != #output terminals";

    register_input_terminals(input_terminals, innames);
    register_output_terminals(output_terminals, outnames);

    register_input_callbacks(std::make_index_sequence<numins>{});

    this->process_pending();
  }

  Op(const input_edges_type& inedges, const output_edges_type& outedges,
     const std::string& name, const std::vector<std::string>& innames,
     const std::vector<std::string>& outnames,
     std::shared_ptr<WorldDCPmapInterface<keyT>> pmap = std::make_shared<WorldDCDefaultPmap<keyT>>(get_default_world()))
      : ::ttg::OpBase(name, numins, numouts),
        worldobjT(get_default_world()),
        world(get_default_world()),
        pmap(pmap) {
    // Cannot call in base constructor since terminals not yet constructed
    if (innames.size() != std::tuple_size<input_terminals_type>::value)
      throw "madness::ttg::Op: #input names != #input terminals";
    if (outnames.size() != std::tuple_size<output_terminalsT>::value)
      throw "madness::ttg::Op: #output names != #output terminals";

    register_input_terminals(input_terminals, innames);
    register_output_terminals(output_terminals, outnames);

    register_input_callbacks(std::make_index_sequence<numins>{});

    connect_my_inputs_to_incoming_edge_outputs(
        std::make_index_sequence<numins>{}, inedges);
    connect_my_outputs_to_outgoing_edge_inputs(
        std::make_index_sequence<numouts>{}, outedges);

    this->process_pending();
  }

  // Destructor checks for unexecuted tasks
  virtual ~Op() {
    if (cache.size() != 0) {
      std::cerr << world.rank() << ":"
                << "warning: unprocessed tasks in destructor of operation '"
                << get_name() << "'" << std::endl;
      std::cerr << world.rank() << ":"
                << "   T => argument assigned     F => argument unassigned"
                << std::endl;
      int nprint = 0;
      for (auto item : cache) {
        if (nprint++ > 10) {
          std::cerr << "   etc." << std::endl;
          break;
        }
        std::cerr << world.rank() << ":"
                  << "   unused: " << item.first << " : ( ";
        for (std::size_t i = 0; i < numins; i++)
          std::cerr << (item.second->argset[i] ? "T" : "F") << " ";
        std::cerr << ")" << std::endl;
      }
    }
  }

  /// Waits for the entire TTG associated with this op to be completed (collective)

  /// This is a collective operation and must be invoked by the main
  /// thread on all processes.  In the MADNESS implementation it
  /// fences the entire world associated with the TTG.  If you wish to
  /// fence TTGs independently, then give each its own world.
  void fence() {
      world.gop.fence();
  }

  // Returns pointer to input terminal i to facilitate connection --- terminal
  // cannot be copied, moved or assigned
  template <std::size_t i>
  typename std::tuple_element<i, input_terminals_type>::type* in() {
    return &std::get<i>(input_terminals);
  }

  // Returns pointer to output terminal for purpose of connection --- terminal
  // cannot be copied, moved or assigned
  template <std::size_t i>
  typename std::tuple_element<i, output_terminalsT>::type* out() {
    return &std::get<i>(output_terminals);
  }

  // Manual injection of a task with all input arguments specified as a tuple
  void invoke(const keyT& key, const input_values_tuple_type& args) {
    set_args(std::make_index_sequence<
                 std::tuple_size<input_values_tuple_type>::value>{},
             key, args);
  }

  // Manual injection of a task that has no arguments
  void invoke(const keyT& key) { set_arg_empty(key); }
};

// Class to wrap a callable with signature
//
// void op(const input_keyT&, const std::tuple<input_valuesT...>&,
// std::tuple<output_terminalsT...>&)
//
template <typename funcT, typename keyT, typename output_terminalsT,
          typename... input_valuesT>
class WrapOp
    : public Op<keyT, output_terminalsT,
                WrapOp<funcT, keyT, output_terminalsT, input_valuesT...>,
                input_valuesT...> {
  using baseT = Op<keyT, output_terminalsT,
                   WrapOp<funcT, keyT, output_terminalsT, input_valuesT...>,
                   input_valuesT...>;
  funcT func;

 public:
  WrapOp(const funcT& func, const typename baseT::input_edges_type& inedges,
         const typename baseT::output_edges_type& outedges,
         const std::string& name, const std::vector<std::string>& innames,
         const std::vector<std::string>& outnames)
      : baseT(inedges, outedges, name, innames, outnames), func(func) {}

  void op(const keyT& key, const typename baseT::input_values_tuple_type& args,
          output_terminalsT& out) {
    func(key, args, out);
  }
};

// Class to wrap a callable with signature
//
// void op(const input_keyT&, const std::tuple<input_valuesT...>&,
// std::tuple<output_terminalsT...>&)
//
template <typename funcT, typename keyT, typename output_terminalsT,
          typename... input_valuesT>
class WrapOpArgs
    : public Op<keyT, output_terminalsT,
                WrapOpArgs<funcT, keyT, output_terminalsT, input_valuesT...>,
                input_valuesT...> {
  using baseT = Op<keyT, output_terminalsT,
                   WrapOpArgs<funcT, keyT, output_terminalsT, input_valuesT...>,
                   input_valuesT...>;
  funcT func;

  template <std::size_t... S>
  void call_func_from_tuple(const keyT& key,
                            const typename baseT::input_values_tuple_type& args,
                            output_terminalsT& out, std::index_sequence<S...>) {
    func(key, std::get<S>(args)..., out);
  }

 public:
  WrapOpArgs(const funcT& func, const typename baseT::input_edges_type& inedges,
             const typename baseT::output_edges_type& outedges,
             const std::string& name, const std::vector<std::string>& innames,
             const std::vector<std::string>& outnames)
      : baseT(inedges, outedges, name, innames, outnames), func(func) {}

  void op(const keyT& key, const typename baseT::input_values_tuple_type& args,
          output_terminalsT& out) {
    call_func_from_tuple(
        key, args, out,
        std::make_index_sequence<
            std::tuple_size<typename baseT::input_values_tuple_type>::value>{});
  };
};

// Factory function to assist in wrapping a callable with signature
//
// void op(const input_keyT&, const std::tuple<input_valuesT...>&,
// std::tuple<output_terminalsT...>&)
template <typename keyT, typename funcT, typename... input_valuesT,
          typename... output_edgesT>
auto wrapt(
    const funcT& func,
    const std::tuple<::ttg::Edge<keyT, input_valuesT>...>& inedges,
    const std::tuple<output_edgesT...>& outedges,
    const std::string& name = "wrapper",
    const std::vector<std::string>& innames = std::vector<std::string>(
        std::tuple_size<std::tuple<::ttg::Edge<keyT, input_valuesT>...>>::value,
        "input"),
    const std::vector<std::string>& outnames = std::vector<std::string>(
        std::tuple_size<std::tuple<output_edgesT...>>::value, "output")) {
  using input_terminals_type = std::tuple<
      typename ::ttg::Edge<keyT, input_valuesT>::input_terminal_type...>;
  using output_terminals_type = typename ::ttg::edges_to_output_terminals<
      std::tuple<output_edgesT...>>::type;
  using callable_type =
      std::function<void(const keyT&, const std::tuple<input_valuesT...>&,
                         output_terminals_type&)>;
  callable_type f(func);  // pimarily to check types
  using wrapT = WrapOp<funcT, keyT, output_terminals_type, input_valuesT...>;

  return std::make_unique<wrapT>(func, inedges, outedges, name, innames,
                                 outnames);
}

// Factory function to assist in wrapping a callable with signature
//
// void op(const input_keyT&, input_valuesT&...,
// std::tuple<output_terminalsT...>&)
template <typename keyT, typename funcT, typename... input_valuesT,
          typename... output_edgesT>
auto wrap(
    const funcT& func,
    const std::tuple<::ttg::Edge<keyT, input_valuesT>...>& inedges,
    const std::tuple<output_edgesT...>& outedges,
    const std::string& name = "wrapper",
    const std::vector<std::string>& innames = std::vector<std::string>(
        std::tuple_size<std::tuple<::ttg::Edge<keyT, input_valuesT>...>>::value,
        "input"),
    const std::vector<std::string>& outnames = std::vector<std::string>(
        std::tuple_size<std::tuple<output_edgesT...>>::value, "output")) {
  using input_terminals_type = std::tuple<
      typename ::ttg::Edge<keyT, input_valuesT>::input_terminal_type...>;
  using output_terminals_type = typename ::ttg::edges_to_output_terminals<
      std::tuple<output_edgesT...>>::type;
  using wrapT =
      WrapOpArgs<funcT, keyT, output_terminals_type, input_valuesT...>;

  return std::make_unique<wrapT>(func, inedges, outedges, name, innames,
                                 outnames);
}

}  // namespace ttg
}  // namespace madness

#endif  // MADNESS_TTG_H_INCLUDED
