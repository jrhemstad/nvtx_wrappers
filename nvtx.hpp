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

using Color = uint32_t;

/**---------------------------------------------------------------------------*
 * @brief Used for grouping NVTX events such as a `Mark` or `NestedRange`.
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
  EventAttributes(std::string const& message, Color color,
                  Category category = {}) noexcept
      : _attributes{0} {
    _attributes.version = NVTX_VERSION;
    _attributes.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    _attributes.category = category.id();
    _attributes.colorType = NVTX_COLOR_ARGB;
    _attributes.color = color;
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
 * @brief A RAII object for creating a nested NVTX range.
 *
 * When constructed, begins a nested NVTX range. Upon destruction, ends the NVTX
 * range.
 *
 * NestedRange is not default constructible.
 *
 * A NestedRange object may be move constructed or move assigned, but may not be
 * copy constructed nor copy assigned.
 *
 * NestedRanges may be nested within other ranges.
 *
 * Example:
 * ```
 * {
 *    nvtx::NestedRange r0{"range 0"};
 *    some_function();
 *    {
 *       nvtx::NestedRange r1{"range 1"};
 *       other_function();
 *       // Range started in r1 ends when r1 goes out of scope
 *    }
 *    // Ranged started in r0 ends when r0 goes out of scope
 * }
 * ```
 *---------------------------------------------------------------------------**/
class NestedRange {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a NestedRange, beginning an NVTX range event.
   *
   * @throws std::runtime_error If beginning the range failed
   *
   * @param message Message associated with the range.
   *---------------------------------------------------------------------------**/
  explicit NestedRange(std::string const& message) {
    if (nvtxRangePushA(message.c_str()) < 0) {
      throw std::runtime_error{"Failed to start NVTX range: " + message};
    }
  }

  /**---------------------------------------------------------------------------*
   * @brief Construct a NestedRange, beginning an NVTX range event with a custom
   * color and optional category.
   *
   * @throws std::runtime_error If beginning the range failed
   *
   * @param message Messaged associated with the range.
   * @param color Color used to visualize the range.
   * @param category Optional, Category to group the range into.
   *---------------------------------------------------------------------------**/
  NestedRange(std::string const& message, Color color, Category category = {}) {
    EventAttributes attributes{message, color, category};
    if (nvtxRangePushEx(attributes) < 0) {
      throw std::runtime_error{"Failed to start NVTX range: " + message};
    }
  }

  NestedRange() = delete;
  NestedRange(NestedRange const&) = delete;
  NestedRange& operator=(NestedRange const&) = delete;
  NestedRange(NestedRange&&) = default;
  NestedRange& operator=(NestedRange&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the NestedRange, ending the NVTX range event.
   *---------------------------------------------------------------------------**/
  ~NestedRange() noexcept {
    auto result = nvtxRangePop();
    assert(result >= 0);
  }
};

/**---------------------------------------------------------------------------*
 * @brief An object for marking an instantenous event.
 *
 * A Mark is not default constructible.
 *
 * A Mark cannot be copy/move constructed nor assigned.
 *
 * Example:
 * ```
 * some_function(...){
 *    // A mark event will appear for every invocatin of `some_function`
 *    nvtx::Mark m("some_function was called");
 * }
 * ```
 *---------------------------------------------------------------------------**/
class Mark {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a Mark, indicating an instantaneous event in the
   * application.
   *
   * @param message The message associated with the mark event.
   *---------------------------------------------------------------------------**/
  explicit Mark(std::string const& message) { nvtxMarkA(message.c_str()); }

  /**---------------------------------------------------------------------------*
   * @brief Construct a Mark, indicating an instantaneous event in the
   * application with a color and optional Category.
   *
   * @param message Message associated with the mark event.
   * @param color Color used to visual the event.
   * @param category Optional, Category to group the range into.
   *---------------------------------------------------------------------------**/
  Mark(std::string const& message, Color color, Category category = {}) {
    EventAttributes attributes{message, color, category};
    nvtxMarkEx(attributes);
  }

  Mark() = delete;
  ~Mark() = default;
  Mark(Mark const&) = delete;
  Mark& operator=(Mark const&) = delete;
  Mark(Mark&&) = delete;
  Mark& operator=(Mark&&) = delete;
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
 * Domains are not default constructible.
 *
 * Domains are move constructible and move assignable, but may not be copy
 * constructed nor copy assigned.
 *
 *---------------------------------------------------------------------------**/
class Domain {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a new Domain.
   *
   * Domain's may be passed into `NestedDomainRange`s or `DomainMark`s to
   * globally group those events.
   *
   * @param name A unique name identifying the domain
   *---------------------------------------------------------------------------**/
  Domain(std::string const& name) : _domain{nvtxDomainCreateA(name.c_str())} {}

  Domain() = delete;
  Domain(Domain const&) = delete;
  Domain& operator=(Domain const&) = delete;
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
  nvtxDomainHandle_t _domain;
};

/**---------------------------------------------------------------------------*
 * @brief A RAII object for creating a nested NVTX range within a Domain.
 *
 * When constructed, begins a nested NVTX range in the specified Domain. Upon
 * destruction, ends the NVTX range.
 *
 * NestedDomainRange is not default constructible.
 *
 * A NestedDomainRange object may be move constructed or move assigned, but may
 *not be copy constructed nor copy assigned.
 *
 * NestedDomainRanges may be nested within other ranges.
 *
 * Example:
 * ```
 * nvtx::Domain D{"my domain"};
 * {
 *    nvtx::NestedDomainRange r0{D,"range 0"};
 *    some_function();
 *    {
 *       nvtx::NestedDomainRange r1{D,"range 1"};
 *       other_function();
 *       // Range started in r1 ends when r1 goes out of scope
 *    }
 *    // Ranged started in r0 ends when r0 goes out of scope
 * }
 * ```
 *---------------------------------------------------------------------------**/
class NestedDomainRange {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a NestedDomainRange, beginning an NVTX range event in the
   * specified domain with a custom color and optional category.
   *
   * @throws std::runtime_error If beginning the range failed
   *
   * @param domain The domain in which the range belongs
   * @param message Messaged associated with the range.
   * @param color Color used to visualize the range.
   * @param category Optional, Category to group the range into.
   *---------------------------------------------------------------------------**/
  NestedDomainRange(Domain const& domain, std::string const& message,
                    Color color, Category category = {})
      : _domain{domain} {
    EventAttributes attributes{message, color, category};
    if (nvtxDomainRangePushEx(_domain, attributes) < 0) {
      throw std::runtime_error{"Failed to start NVTX domain range: " + message};
    }
  }

  NestedDomainRange() = delete;
  NestedDomainRange(NestedDomainRange const&) = delete;
  NestedDomainRange& operator=(NestedDomainRange const&) = delete;
  NestedDomainRange(NestedDomainRange&&) = default;
  NestedDomainRange& operator=(NestedDomainRange&&) = default;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the NestedDomainRange, ending the NVTX range event.
   *---------------------------------------------------------------------------**/
  ~NestedDomainRange() noexcept {
    auto result = nvtxDomainRangePop(_domain);
    assert(result >= 0);
  }

 private:
  Domain const& _domain;
};

}  // namespace nvtx
