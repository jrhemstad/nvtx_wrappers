/*
 *  Copyright 2019 Jacob Hemstad
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <nvToolsExt.h>

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace nvtx {

struct argb_color {
  using value_type = uint32_t;
  explicit argb_color(value_type v) : _value{v} {}

  value_type value() const noexcept { return _value; }

 private:
  value_type _value{};
};

/**---------------------------------------------------------------------------*
 * @brief Used for grouping NVTX events such as a `Mark` or `domain_thread_range`.
 *---------------------------------------------------------------------------**/
class Category {
 public:
  using Id = uint32_t;
  /**---------------------------------------------------------------------------*
   * @brief Construct Category for grouping NVTX events.
   *
   * Categories are used to group sets of events. Each Category is identified
   * through a unique `name`. Categories with identical `name`s share a unique
   *`id()`.
   *
   * A `Category` may be passed into any Marker/Range. Events that share a
   * `Category` with an identical `name`s will be grouped together.
   *
   * @note This operation is not threadsafe. Attempting to create two `Category`
   * objects concurrently across 2 or more threads yields undefined behavior.
   *
   * @param name The string used to uniquely identify the Category
   *---------------------------------------------------------------------------**/
  Category(std::string const& name) {
    auto found = _name_to_id.find(name);
    if (found == _name_to_id.end()) {
      _id = ++_id_counter;
      _name_to_id.insert({name, _id});
      nvtxNameCategoryA(_id, name.c_str());
    } else {
      _id = found->second;
    }
  }

  Category() = default;
  ~Category() = default;
  Category(Category const&) = default;
  Category& operator=(Category const&) = default;
  Category(Category&&) = default;
  Category& operator=(Category&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Returns the Category`s numerical id.
   *
   * Categories created with identical `name`s share the same `id()`.
   *
   * @return Id The numerical id associated with the Category's `name`.
   *---------------------------------------------------------------------------**/
  Id id() const noexcept { return _id; }

 private:
  /**---------------------------------------------------------------------------*
   * @brief Maps `name`s to their corresponding unique IDs
   *
   * TODO: This should probably be threadsafe
   *---------------------------------------------------------------------------**/
  static std::unordered_map<std::string, Id> _name_to_id;

  /**---------------------------------------------------------------------------*
   * @brief Counter used to generate new unique ids for Categories with new
   *names.
   *---------------------------------------------------------------------------**/
  static Id _id_counter;

  Id _id{0};  ///< The id of the Category
};

/**---------------------------------------------------------------------------*
 * @brief Describes the attributes of a NVTX event such as color and
 * identifying message.
 *---------------------------------------------------------------------------**/
class EventAttributes {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct an EventAttributes to provide additional description of a
   * NVTX event.
   *
   * @param message The message attached to the event.
   * @param color The color used to visualize the event.
   * @param category Optional, Category to group the event into.
   *---------------------------------------------------------------------------**/
  EventAttributes(std::string const& message, argb_color color,
                  Category category = {}) noexcept
      : _attributes{0} {
    _attributes.version = NVTX_VERSION;
    _attributes.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    _attributes.category = category.id();
    _attributes.colorType = NVTX_COLOR_ARGB;
    _attributes.color = color.value();
    _attributes.messageType = NVTX_MESSAGE_TYPE_ASCII;
    _attributes.message.ascii = message.c_str();
  }
  EventAttributes() = delete;
  ~EventAttributes() = default;
  EventAttributes(EventAttributes const&) = default;
  EventAttributes& operator=(EventAttributes const&) = default;
  EventAttributes(EventAttributes&&) = default;
  EventAttributes& operator=(EventAttributes&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Conversion operator to `nvtxEventAttributes_t`.
   *
   * Allows transparently passing an `EventAttributes` object into a function
   * expecting a `nvtxEventAttributes_t` argument.
   *---------------------------------------------------------------------------**/
  operator nvtxEventAttributes_t() const noexcept { return _attributes; }

  /**---------------------------------------------------------------------------*
   * @brief Conversion operator to `nvtxEventAttributes_t*`.
   *
   * Allows transparently passing a `EventAttributes` object into a function
   * expecting a pointer to `nvtxEventAttributes_t` argument.
   *---------------------------------------------------------------------------**/
  operator nvtxEventAttributes_t*() noexcept { return &_attributes; }

 private:
  nvtxEventAttributes_t _attributes{};
};

/**---------------------------------------------------------------------------*
 * @brief An object for grouping events into a global scope.
 *
 * Domains are used to group events to a developer defined scope. Middleware
 * vendors may also scope their own events to avoid collisions with the
 * the application developer's events, so that the application developer may
 * inspect both parts and easily differentiate or filter them.  By default
 * all events are scoped to a global domain where NULL is provided or when
 * using APIs provided b versions of NVTX below v2
 *
 * Domains are intended to be typically long lived objects with the intention
 * of logically separating events of large modules from each other such as
 * middleware libraries from each other and the main application.
 *
 * Domains are coarser-grained than `Category`s. It is common for a library to
 * have only a single Domain, which may be further sub-divided into `Category`s.
 *
 *
 *---------------------------------------------------------------------------**/
class Domain {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a new Domain.
   *
   * Domain's may be passed into `domain_thread_range` or `mark` to  globally group
   * those events.
   *
   * @param name A unique name identifying the domain
   *---------------------------------------------------------------------------**/
  explicit Domain(const char* name) : _domain{nvtxDomainCreateA(name)} {}

  /**---------------------------------------------------------------------------*
   * @brief Construct a new Domain.
   *
   * Domain's may be passed into `domain_thread_range` or `mark` to  globally group
   * those events.
   *
   * @param name A unique name identifying the domain
   *---------------------------------------------------------------------------**/
  explicit Domain(std::string const& name) : Domain{name.c_str()} {}

  Domain() = default;
  Domain(Domain const&) = default;
  Domain& operator=(Domain const&) = default;
  Domain(Domain&&) = default;
  Domain& operator=(Domain&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the Domain object, unregistering and freeing all domain
   * specific resources.
   *---------------------------------------------------------------------------**/
  ~Domain() { nvtxDomainDestroy(_domain); }

  /**---------------------------------------------------------------------------*
   * @brief Conversion operator to `nvtxDomainHandle_t`.
   *
   * Allows transparently passing a Domain object into an API expecting a native
   * `nvtxDomainHandle_t` object.
   *---------------------------------------------------------------------------**/
  operator nvtxDomainHandle_t() const noexcept { return _domain; }

 private:
  nvtxDomainHandle_t _domain{};
};

/**---------------------------------------------------------------------------*
 * @brief A RAII object for creating a NVTX range local to a thread.
 *
 * When constructed, begins a nested NVTX range on the calling thread. Upon
 * destruction, ends the NVTX range.
 *
 * Behavior is undefined if a `domain_thread_range` object is created/destroyed on
 * different threads.
 *
 * `domain_thread_range` is not default constructible.
 *
 * `domain_thread_range` is neither moveable nor copyable.
 *
 * `domain_thread_range`s may be nested within other ranges.
 *
 * Example:
 * ```
 * {
 *    nvtx::domain_thread_range r0{"range 0"};
 *    some_function();
 *    {
 *       nvtx::domain_thread_range r1{"range 1"};
 *       other_function();
 *       // Range started in r1 ends when r1 goes out of scope
 *    }
 *    // Ranged started in r0 ends when r0 goes out of scope
 * }
 * ```
 *---------------------------------------------------------------------------**/
class domain_thread_range {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a domain_thread_range, beginning an NVTX range event.
   *
   * @param message Message associated with the range.
   *---------------------------------------------------------------------------**/
  explicit domain_thread_range(std::string const& message) {
    nvtxRangePushA(message.c_str());
  }

  /**---------------------------------------------------------------------------*
   * @brief Construct a domain_thread_range, beginning an NVTX range event with a
   *custom color and optional category.
   *
   * @param message Message associated with the range.
   * @param color Color used to visualize the range.
   * @param category Optional, Category to group the range into.
   *---------------------------------------------------------------------------**/
  domain_thread_range(std::string const& message, argb_color color,
               Category category = {}, Domain domain = {})
      : _domain{domain} {
    nvtxDomainRangePushEx(_domain, EventAttributes{message, color, category});
  }

  domain_thread_range() = delete;
  domain_thread_range(domain_thread_range const&) = delete;
  domain_thread_range& operator=(domain_thread_range const&) = delete;
  domain_thread_range(domain_thread_range&&) = delete;
  domain_thread_range& operator=(domain_thread_range&&) = delete;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the domain_thread_range, ending the NVTX range event.
   *---------------------------------------------------------------------------**/
  ~domain_thread_range() noexcept { nvtxDomainRangePop(_domain); }

 private:
  Domain _domain{};  ///< Optional domain in which the range lives
};

template <class D>
class DomainSingleton {
 public:
  static Domain const& get_instance() {
    static Domain instance{D::name};
    return instance;
  }
  DomainSingleton() = delete;
  DomainSingleton(DomainSingleton const&) = delete;
};


/**
 * @brief Indicates an instantaneous event.
 *
 * Example:
 * ```c++
 * some_function(...){
 *    // A mark event will appear for every invocation of `some_function`
 *    nvtx::mark("some_function was called");
 * }
 * ```
 * @param message Message associated with the range.
 */
void mark(std::string const& message) { nvtxMarkA(message.c_str()); }

/**
 * @brief Indicates an instantaneous event.
 *
 * @param message Message associated with the `mark`
 * @param color color used to visualize the `mark`
 * @param category Optional, Category to group the `mark` into.
 */
void mark(std::string const& message, argb_color color, Category category = {},
          Domain domain = {}) {
  nvtxDomainMarkEx(domain, EventAttributes{message, color, category});
}

}  // namespace nvtx
