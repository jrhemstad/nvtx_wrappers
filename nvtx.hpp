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
      _id = ++next_id;
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
  static Id _id_counter{0};

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
    _attributes.message = message.c_str();
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
  operator nvtxEventAttributes_t() { return _attributes; }

 private:
  nvtxEventAttributes_t _attributes{};
};

/**---------------------------------------------------------------------------*
 * @brief A RAII object for creating a nested NVTX range.
 *
 * When constructed, begins a nested NVTX range. Upon destruction, ends the NVTX
 * range.
 *
 *---------------------------------------------------------------------------**/
class NestedRange {
  explicit NestedRange(std::string const& message) {
    if (nvtxRangePushA(message.c_str()) < 0) {
      throw std::runtime_error{"Failed to start NVTX range: " + message};
    }
  }

  NestedRange(std::string const& message, Color color, Category category = {}) {
    if (nvtxRangePushEx({message, color, category}) < 0) {
      throw std::runtime_error{"Failed to start NVTX range: " + message};
    }
  }

  NestedRange() = delete;
  NestedRange(NestedRange const&) = delete;
  NestedRange& operator=(NestedRange const&) = delete;
  NestedRange(NestedRange&&) = default;
  NestedRange& operator(NestedRange&&) = default;

  ~NestedRange() noexcept {
    auto result = nvtxRangePop();
    assert(result >= 0);
  }
};

}  // namespace nvtx
