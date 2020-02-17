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

#include <cxxabi.h>
#include <cassert>
#include <iostream>
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
   * @brief Construct a new Domain with the specified `name`.
   *
   * @param name A unique name identifying the domain
   *---------------------------------------------------------------------------**/
  explicit Domain(const char* name) noexcept
      : _domain{nvtxDomainCreateA(name)} {}

  /**---------------------------------------------------------------------------*
   * @brief Construct a new Domain with the specified `name`.
   *
   * @param name A unique name identifying the domain
   *---------------------------------------------------------------------------**/
  explicit Domain(const wchar_t* name) noexcept
      : _domain{nvtxDomainCreateW(name)} {}

  /**---------------------------------------------------------------------------*
   * @brief Construct a new Domain with the specified `name`.
   *
   * @param name A unique name identifying the domain
   *---------------------------------------------------------------------------**/
  explicit Domain(std::string const& name) noexcept : Domain{name.c_str()} {}

  Domain() = default;
  Domain(Domain const&) = delete;
  Domain& operator=(Domain const&) = delete;
  Domain(Domain&&) = delete;
  Domain& operator=(Domain&&) = delete;

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

/**
 * @brief Tag type for the "global" NVTX domain.
 *
 * This type may be passed as a template argument to any function expecting a
 * `Domain` argument to indicate that the global domain should be used.
 *
 * The global NVTX domain is the same as using no domain at all.
 *
 */
struct global_domain_tag {};

namespace detail {

/**---------------------------------------------------------------------------*
 * @brief Return a string of the demangled name of a type `T`
 *
 * This is useful for debugging Type list utilities.
 *
 * @tparam T The type whose name is returned as a string
 * @return std::string The demangled name of `T`
 *---------------------------------------------------------------------------**/
template <typename T>
std::string type_name() {
  int status;
  char* realname;
  realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
  std::string name{realname};
  free(realname);
  return name;
}

/**
 * @brief Verifies if a type `T` contains a member `T::name` of type `const
 * char*` or `const wchar_t*`.
 *
 * @tparam T The type to verify
 * @return True if `T` contains a member `T::name` of type `const char*` or
 * `const wchar_t*`.
 */
template <typename T>
constexpr auto has_name_member() noexcept -> decltype(T::name, bool()) {
  return (std::is_same<char const*,
                       typename std::decay<decltype(T::name)>::type>::value or
          std::is_same<wchar_t const*,
                       typename std::decay<decltype(T::name)>::type>::value);
}
}  // namespace detail

/**
 * @brief Returns instance of a `Domain` constructed using the specified
 * name created as a function local static.
 *
 * Uses the "construct on first use" idiom to safely ensure the Domain object
 * is initialized exactly once. See
 * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 *
 * The Domain's name is specified via template parameter `D`. `D` is required
 * to be a type that contains a `const char*` or `const wchar_t*` member named
 * `name`.
 *
 * @tparam D Type that contains a `D::name` member of type `const char*` or
 * `const whchar*`
 * @return Reference to the `Domain` created with the specified name.
 */
template <class D>
Domain const& get_domain() {
  // static_assert(detail::has_name_member<D>(),
  //              "Type used to identify a Domain must contain a name member of
  //              " "type const char* or const wchar_t*");
  static Domain d{D::name};
  return d;
}

/**
 * @brief Returns reference to the global `Domain`.
 *
 * This specialization for `global_domain_tag` returns a default constructed,
 * null `Domain` object for use when the "global" (i.e., no domain) is desired.
 *
 * @return Reference to a default constructed, null `Domain` object.
 */
template <>
Domain const& get_domain<global_domain_tag>() {
  static Domain d{};
  return d;
}

/**
 * @brief Uniquely identifies a `Category` object.
 *
 */
struct category_id {
  using value_type = uint32_t;

  /**
   * @brief Construct a `category_id` with the specified `value`.
   *
   */
  constexpr explicit category_id(value_type value) noexcept : value_{value} {}

  constexpr explicit operator value_type() const noexcept {
    return get_value();
  }

  /**
   * @brief Returns the `category_id`'s value
   */
  constexpr value_type get_value() const noexcept { return value_; }

 private:
  value_type value_{};
};

/**---------------------------------------------------------------------------*
 * @brief Used for grouping NVTX events such as a `Mark` or
 * `domain_thread_range`.
 *
 * A `Category` allows for more fine-grain grouping of NVTX events than a
 * `Domain`. While it is typical for a library to only have a single `Domain`,
 * it may have several `Category`s. For example, one might have separate
 * categories for IO, memory allocation, compute, etc.
 *
 * @tparam Domain Type containing `name` member used to identify the `Domain` to
 * which the `Category` belongs. Else, `global_domain_tag` to  indicate that the
 * global NVTX domain should be used.
 *---------------------------------------------------------------------------**/
template <typename Domain = nvtx::global_domain_tag>
class Category {
 public:
  /**
   * @brief Construct a `Category` with the specified `id`.
   *
   * The `Category` will be unnamed and identified only by it's `id` value.
   *
   */
  constexpr explicit Category(category_id id) noexcept : id_{id} {}

  /**
   * @brief Construct a `Category` with the specified `id` and `name`.
   *
   * The name `name` will be registered with `id`.
   *
   */
  constexpr Category(category_id id, const char* name) noexcept : id_{id} {
    nvtxDomainNameCategoryA(get_domain<Domain>(), id_.get_value(), name);
  };

  /**
   * @brief Construct a `Category` with the specified `id` and `name`.
   *
   * The name `name` will be registered with `id`.
   *
   */
  constexpr Category(category_id id, const wchar_t* name) noexcept : id_{id} {
    nvtxDomainNameCategoryW(get_domain<Domain>(), id_.get_value(), name);
  };
  Category() = delete;
  ~Category() = default;
  Category(Category const&) = delete;
  Category& operator=(Category const&) = delete;
  Category(Category&&) = delete;
  Category& operator=(Category&&) = delete;

 private:
  category_id id_{};  ///< The Category's id
};

/**
 * @brief Returns a global instance of a `Category` as a function-local static.
 *
 * Creates the `Category` with the name specified by `Name`, id specified by
 * `Id`, and domain by `Domain`.
 *
 * Uses the "construct on first use" idiom to safely ensure the `Category`
 * object is initialized exactly once. See
 * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 *
 * @tparam Name Type that containing a member `Name::name` of type `const char*`
 * or `const wchar_t*` used to name the `Category`.
 * @tparam Id Used to uniquely identify the `Category` and differentiate it from
 * other `Category` objects.
 * @tparam Domain Type containing `name` member used to identify the `Domain` to
 * which the `Category` belongs. Else, `global_domain_tag` to  indicate that the
 * global NVTX domain should be used.
 */
template <typename Name, uint32_t Id, typename Domain = nvtx::global_domain_tag>
Category<Domain> const& get_category() noexcept {
  static_assert(detail::has_name_member<Name>(),
                "Type used to name a Category must contain a name member of "
                "type const char* or const wchar_t*");
  static Category<Domain> category{Name::name, category_id{Id}};
  return category;
}

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

  template <class D = nvtx::global_domain_tag>
  EventAttributes(std::string const& message, argb_color color,
                  Category<D> category = {}) noexcept
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

/**
 * @brief A RAII object for creating a NVTX range local to a thread within a
 * domain.
 *
 * When constructed, begins a nested NVTX range on the calling thread in the
 * specified domain. Upon destruction, ends the NVTX range.
 *
 * Behavior is undefined if a `domain_thread_range` object is created/destroyed
 * on different threads.
 *
 * `domain_thread_range` is not default constructible.
 *
 * `domain_thread_range` is neither moveable nor copyable.
 *
 * `domain_thread_range`s may be nested within other ranges.
 *
 * The domain of the range is specified by the template type parameter `D`.
 * By default, the `global_domain_tag` is used, which scopes the range to the
 * global NVTX domain. The convenience alias `thread_range` is provided for
 * ranges scoped to the global domain.
 *
 * If a custom domain is desired, a custom type may be specified. This type is
 * required to contain a `static constexpr const char*` member called `name`.
 * For example:
 * ```c++
 * struct my_domain{
 *    static constexpr const char * name{"my domain"};
 * };
 * ```
 *
 * Example usage:
 * ```c++
 * nvtx::domain_thread_range<> r0{"range 0"}; // Range in global domain
 *
 * nvtx::thread_range r1{"range 1"}; // Alias for range in global domain
 *
 * nvtx::domain_thread_range<my_domain> r2{"range 2"}; // Range in custom domain
 *
 * // specify an alias to a range that uses a custom domain
 * using my_thread_range = nvtx::domain_thread_range<my_domain>;
 *
 * my_thread_range r3{"range 3"}; // Alias for range in custom domain
 * ```
 */
template <class D = nvtx::global_domain_tag>
class domain_thread_range {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Construct a domain_thread_range, beginning an NVTX range event with
   * a custom color and optional category.
   *
   * @param message Message associated with the range.
   * @param color Color used to visualize the range.
   * @param category Optional, Category to group the range into.
   *---------------------------------------------------------------------------**/
  domain_thread_range(std::string const& message, argb_color color) {
    // nvtxDomainRangePushEx(detail::get_domain<D>(),
    //                       EventAttributes{message, color, category});
  }

  domain_thread_range() = delete;
  domain_thread_range(domain_thread_range const&) = delete;
  domain_thread_range& operator=(domain_thread_range const&) = delete;
  domain_thread_range(domain_thread_range&&) = delete;
  domain_thread_range& operator=(domain_thread_range&&) = delete;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the domain_thread_range, ending the NVTX range event.
   *---------------------------------------------------------------------------**/
  ~domain_thread_range() noexcept { nvtxDomainRangePop(get_domain<D>()); }
};

/**
 * @brief Convenience alias for a `thread_range` in the global NVTX domain.
 *
 */
using thread_range = domain_thread_range<>;

}  // namespace nvtx
