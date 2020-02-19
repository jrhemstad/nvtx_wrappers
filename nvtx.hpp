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
#include <limits>
#include <string>

// Initializing a legacy-C (i.e., no constructor) union member requires
// initializing in the constructor body. Non-empty constexpr constructors
// require C++14 relaxed constexpr.
#if __cpp_constexpr >= 201304L
#define NVTX_RELAXED_CONSTEXPR constexpr
#else
#define NVTX_RELAXED_CONSTEXPR
#endif

namespace nvtx {

/**
 * @brief Indicates the values of the red, green, blue color channels for
 * a RGB color code.
 *
 */
struct RGB {
  using component_type = uint8_t;

  /**
   * @brief Construct a RGB with red, green, and blue channels
   * specified by `red_`, `green_`, and `blue_`, respectively.
   *
   * Valid values are in the range `[0,255]`.
   *
   */
  constexpr RGB(component_type red_, component_type green_,
                component_type blue_) noexcept
      : red{red_}, green{green_}, blue{blue_} {}

  component_type const red{};    ///< Red channel value
  component_type const green{};  ///< Green channel value
  component_type const blue{};   ///< Blue channel value
};

/**
 * @brief Indicates the value of the alpha, red, green, and blue color channels
 * for an ARGB color code.
 *
 */
struct ARGB : RGB {
  using component_type = typename RGB::component_type;

  /**
   * @brief Construct an ARGB with alpha, red, green, and blue channels
   * specified by `alpha_`, `red_`, `green_`, and `blue_`, respectively.
   *
   * Valid values are in the range `[0,255]`.
   *
   */
  constexpr ARGB(component_type alpha_, component_type red_,
                 component_type green_, component_type blue_) noexcept
      : RGB{red_, green_, blue_}, alpha{alpha_} {}

  component_type const alpha{};  ///< Alpha channel value
};

/**
 * @brief Represents a color that can be associated with NVTX events.
 *
 */
class Color {
 public:
  using value_type = uint32_t;

  /**
   * @brief Constructs a `Color` using the values provided by `hex_code`.
   *
   * `hex_code` is expected to be a 4 byte ARGB hex code.
   *
   * The most significant byte indicates the value of the alpha channel
   * (opacity) (0-255)
   *
   * The next byte indicates the value of the red channel (0-255)
   *
   * The next byte indicates the value of the green channel (0-255)
   *
   * The least significant byte indicates the value of the blue channel (0-255)
   *
   * @param v The hex code used to construct the `Color`
   */
  constexpr explicit Color(value_type hex_code) noexcept : _value{hex_code} {}

  /**
   * @brief Construct a `Color` using the alpha, red, green, blue components in
   * `argb`.
   *
   * TODO: Make implicit?
   *
   * @param argb The alpha, red, green, blue components of the desired `Color`
   */
  constexpr explicit Color(ARGB argb) noexcept
      : Color{from_bytes_msb_to_lsb(argb.alpha, argb.red, argb.green,
                                    argb.blue)} {}

  /**
   * @brief Construct a `Color` using the red, green, blue components in
   * `rgb`.
   *
   * Uses maximum value for the alpha channel (opacity) of the `Color`.
   *
   * TODO: Make implicit?
   *
   * @param rgb The red, green, blue components of the desired `Color`
   */
  constexpr explicit Color(RGB rgb) noexcept
      : Color{from_bytes_msb_to_lsb(0xFF, rgb.red, rgb.green, rgb.blue)} {}

  /**
   * @brief Returns the `Color`s ARGB hex code
   *
   */
  constexpr value_type get_value() const noexcept { return _value; }

  /**
   * @brief Return the NVTX color type of the Color.
   *
   */
  constexpr nvtxColorType_t get_type() const noexcept { return _type; }

 private:
  /**
   * @brief Constructs an unsigned, 4B integer from the component bytes in most
   * to least significant byte order.
   *
   */
  constexpr static value_type from_bytes_msb_to_lsb(uint8_t byte3,
                                                    uint8_t byte2,
                                                    uint8_t byte1,
                                                    uint8_t byte0) {
    return uint32_t{byte3} << 24 | uint32_t{byte2} << 16 |
           uint32_t{byte1} << 8 | uint32_t{byte0};
  }

  value_type const _value{};                     ///< Color's ARGB color code
  nvtxColorType_t const _type{NVTX_COLOR_ARGB};  ///< NVTX color type code
};

/**---------------------------------------------------------------------------*
 * @brief An object for grouping events into a global scope.
 *
 * Domains are used to group events to a developer defined scope. Middleware
 * vendors may also scope their own events to avoid collisions with the
 * the application developer's events, so that the application developer may
 * inspect both parts and easily differentiate or filter them.
 *
 * Domains are intended to be typically long lived objects with the intention
 * of logically separating events of large modules from each other such as
 * middleware libraries from each other and the main application.
 *
 * Domains are coarser-grained than `Category`s. It is typical for a library to
 * have only a single Domain, which may be further sub-divided into `Category`s.
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
Domain const& get_domain() noexcept {
  static_assert(detail::has_name_member<D>(),
                "Type used to identify a Domain must contain a name member of"
                "type const char* or const wchar_t*");
  static Domain const d{D::name};
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
Domain const& get_domain<global_domain_tag>() noexcept {
  static Domain const d{};
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

  /**
   * @brief Returns the `category_id`'s value
   */
  constexpr value_type get_value() const noexcept { return value_; }

 private:
  value_type const value_{};
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

  /**
   * @brief Returns the id of the Category.
   *
   */
  constexpr category_id get_id() const noexcept { return id_; }

  Category() = delete;
  ~Category() = default;
  Category(Category const&) = delete;
  Category& operator=(Category const&) = delete;
  Category(Category&&) = delete;
  Category& operator=(Category&&) = delete;

 private:
  category_id const id_{};  ///< Category's unique identifier
};

/**
 * @brief Returns a global instance of a `Category` as a function-local static.
 *
 * Creates the `Category` with the name specified by `Name`, id specified by
 * `Id`, and domain by `Domain`. This function is useful for constructing a
 * named `Category` exactly once and reusing the same instance throughout an
 * application.
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
  static Category<Domain> const category{Name::name, category_id{Id}};
  return category;
}

/**
 * @brief Represents a message registered with NVTX.
 *
 * Message registration is an optimization to lower the overhead of associating
 * a message with an NVTX event by avoiding copying the contents of the message
 * for each event.
 *
 * Registering a message yields a handle that may be used with any NVTX event.
 *
 * @tparam Domain Type containing `name` member used to identify the `Domain` to
 * which the `RegisteredMessage` belongs. Else, `global_domain_tag` to  indicate
 * that the global NVTX domain should be used.
 */
template <typename Domain = nvtx::global_domain_tag>
class RegisteredMessage {
 public:
  constexpr explicit RegisteredMessage(char const* msg) noexcept
      : value_{nvtxDomainRegisterStringA(get_domain<Domain>(), msg)} {}

  constexpr explicit RegisteredMessage(wchar_t const* msg) noexcept
      : value_{nvtxDomainRegisterStringW(get_domain<Domain>(), msg)} {}

  constexpr nvtxStringHandle_t get_handle() const noexcept { return value_; }

 private:
  nvtxStringHandle_t value_{};
};

/**
 * @brief Returns a global instance of a `RegisteredMessage` as a function local
 * static.
 *
 * Upon first invocation, constructs a `RegisteredMessage` whose contents are
 * specified by `Message::message` in the domain `Domain`.
 *
 * All future invocations will return a reference to the object constructed in
 * the first invocation.
 *
 * This function is meant to provide a convenient way to register a message with
 * NVTX without having to explicitly register the message.
 *
 * @tparam Message Type containing `message` member used as the message's
 * contents.
 * @tparam Domain Type containing `name` member used to identify the `Domain` to
 * which the `RegisteredMessage` belongs. Else, `global_domain_tag` to  indicate
 * that the global NVTX domain should be used.
 */
template <typename Message, typename Domain = nvtx::global_domain_tag>
RegisteredMessage<Domain> const& get_registered_message() noexcept {
  static RegisteredMessage<Domain> const registered_message{Message::message};
  return registered_message;
}

/**
 * @brief Allows associating a message string with an NVTX event.
 *
 */
class Message {
 public:
  using value_type = nvtxMessageValue_t;

  /**
   * @brief Construct a `Message` whose contents are specified by `msg`.
   *
   * @param msg The contents of the message
   */
  NVTX_RELAXED_CONSTEXPR Message(char const* msg) noexcept
      : type_{NVTX_MESSAGE_TYPE_ASCII} {
    value_.ascii = msg;
  }

  /**
   * @brief Construct a `Message` whose contents are specified by `msg`.
   *
   * @param msg The contents of the message
   */
  NVTX_RELAXED_CONSTEXPR Message(wchar_t const* msg) noexcept
      : type_{NVTX_MESSAGE_TYPE_UNICODE} {
    value_.unicode = msg;
  }

  /**
   * @brief Construct a `Message` from a `RegisteredMessage`.
   *
   * @tparam Domain Type containing `name` member used to identify the `Domain`
   * to which the `RegisteredMessage` belongs. Else, `global_domain_tag` to
   * indicate that the global NVTX domain should be used.
   * @param msg The message that has already been registered with NVTX.
   */
  template <typename Domain>
  NVTX_RELAXED_CONSTEXPR Message(RegisteredMessage<Domain> msg) noexcept
      : type_{NVTX_MESSAGE_TYPE_REGISTERED} {
    value_.registered = msg.get_handle();
  }

  /**
   * @brief Return the union holding the value of the message.
   *
   */
  constexpr value_type get_value() const noexcept { return value_; }

  /**
   * @brief Return the type information about the value the union holds.
   *
   */
  constexpr nvtxMessageType_t get_type() const noexcept { return type_; }

 private:
  nvtxMessageType_t const type_{};  ///< Message type
  nvtxMessageValue_t value_{};      ///< Message contents
};

/**
 * @brief A user-defined numerical value that can be associated with an NVTX
 * event.
 *
 */
class Payload {
 public:
  using value_type = typename nvtxEventAttributes_v2::payload_t;

  NVTX_RELAXED_CONSTEXPR explicit Payload(int64_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_INT64}, value_{} {
    value_.llValue = value;
  }
  NVTX_RELAXED_CONSTEXPR explicit Payload(int32_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_INT32}, value_{} {
    value_.iValue = value;
  }
  NVTX_RELAXED_CONSTEXPR explicit Payload(uint64_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_UNSIGNED_INT64}, value_{} {
    value_.ullValue = value;
  }
  NVTX_RELAXED_CONSTEXPR explicit Payload(uint32_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_UNSIGNED_INT32}, value_{} {
    value_.uiValue = value;
  }
  NVTX_RELAXED_CONSTEXPR explicit Payload(float value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_FLOAT}, value_{} {
    value_.fValue = value;
  }
  NVTX_RELAXED_CONSTEXPR explicit Payload(double value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_DOUBLE}, value_{} {
    value_.dValue = value;
  }

  /**
   * @brief Return the union holding the value of the payload
   *
   */
  constexpr value_type get_value() const noexcept { return value_; }

  /**
   * @brief Return the information about the type the union holds.
   *
   */
  constexpr nvtxPayloadType_t get_type() const noexcept { return type_; }

 private:
  nvtxPayloadType_t const type_;  ///< Type of the payload value
  value_type value_;              ///< Union holding the payload value
};

/**---------------------------------------------------------------------------*
 * @brief Describes the attributes of a NVTX event such as color and
 * identifying message.
 *---------------------------------------------------------------------------**/
class EventAttributes {
 public:
  /**---------------------------------------------------------------------------*
   * @brief Default constructor creates an `EventAttributes` with no category,
   * color, payload, nor message.
   *---------------------------------------------------------------------------**/
  constexpr EventAttributes() noexcept
      : _attributes{
            NVTX_VERSION,                   // version
            sizeof(nvtxEventAttributes_t),  // size
            0,                              // category
            NVTX_COLOR_UNKNOWN,             // color type
            0,                              // color value
            NVTX_PAYLOAD_UNKNOWN,           // payload type
            {},                             // payload value (union)
            NVTX_MESSAGE_UNKNOWN,           // message type
            {}                              // message value (union)
        } {}

  template <typename Domain, typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Category<Domain> const& c,
                                                  Args&&... args) noexcept
      : EventAttributes(args...) {
    _attributes.category = c.get_id().get_value();
  }

  template <typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Color const& c,
                                                  Args&&... args) noexcept
      : EventAttributes(args...) {
    _attributes.color = c.get_value();
    _attributes.colorType = c.get_type();
  }

  template <typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Payload const& p,
                                                  Args&&... args) noexcept
      : EventAttributes(args...) {
    _attributes.payload = p.get_value();
    _attributes.payloadType = p.get_type();
  }

  template <typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Message const& m,
                                                  Args&&... args) noexcept
      : EventAttributes(args...) {
    _attributes.message = m.get_value();
    _attributes.messageType = m.get_type();
  }

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
  domain_thread_range(std::string const& message, Color color) noexcept {
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
