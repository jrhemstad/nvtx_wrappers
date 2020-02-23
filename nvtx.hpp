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

/**
 * @file nvtx.hpp
 *
 * @brief Provides C++ constructs making the NVTX library safer and easier to
 * use with zero overhead.
 *
 * The NVTX library provides C APIs for users to annotate their code to aid in
 * performance profiling and optimization. One of the most commonly  used NVTX
 * features are "ranges". Ranges allow the user to annotate a span of time which
 * can later be visualized in the application timeline by tools such as Nsight
 * Systems.
 *
 * For example, imagine a user wanted to see every time a function,
 * `my_function`, is called and how long it takes to execute. This can be
 * accomplished with an NVTX range created on the entry to the function and
 * terminated on return from `my_function` using the push/pop C APIs:
 *
 * ```c++
 * void my_function(...){
 *    nvtxRangePushA("my_function"); // Begins NVTX range
 *
 *    // do work
 *
 *    nvtxRangePop(); // Ends NVTX range
 * }
 * ```
 *
 * One of the challenges with using the NVTX C API is that it requires manually
 * terminating the end of the range with `nvtxRangePop`. This can be challenging
 * if `my_function()` has multiple returns or can throw exceptions as it
 * requires calling `nvtxRangePop()` in all of those situations.
 *
 * The C++ wrappers in this header solve this inconvenience (and others) by
 * providing a `thread_range` class using the "RAII" pattern. In short, upon
 * construction `thread_range` calls "push" and upon destruction calls "pop".
 * The above example then becomes:
 *
 * ```c++
 * void my_function(...){
 *    nvtx::thread_range r{"my_function"}; // Begins NVTX range
 *
 *    // do work
 *
 * } // Range ends on exit from `my_function` when `r` is destroyed
 * ```
 *
 * No matter when or where `my_function` returns, through the rules of object
 * lifetime, `r` is guaranteed to be destroyed and end the NVTX range without
 * manual intervention.
 *
 * For more information about NVTX and how it can be used, see
 * https://docs.nvidia.com/cuda/profiler-users-guide/index.html#nvtx and
 * https://devblogs.nvidia.com/cuda-pro-tip-generate-custom-application-profile-timelines-nvtx/
 * for more information.
 *
 *
 */

// Initializing a legacy-C (i.e., no constructor) union member requires
// initializing in the constructor body. Non-empty constexpr constructors
// require C++14 relaxed constexpr.
#if __cpp_constexpr >= 201304L
#define NVTX_RELAXED_CONSTEXPR constexpr
#else
#define NVTX_RELAXED_CONSTEXPR
#endif

namespace nvtx {
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

/**---------------------------------------------------------------------------*
 * @brief `Domain`s allow for grouping NVTX events into a single scope to
 * differentiate them from events in other `Domain`s.
 *
 * By default, all NVTX constructs are placed in the "global" NVTX domain.
 *
 * A custom `Domain` may be used in order to differentiate a library's or
 * application's NVTX events from other events.
 *
 * `Domain`s are expected to be long-lived and unique to a library or
 * application. As such, it is assumed a domain's name is known at compile time.
 * Therefore, all NVTX constructs that can be associated with a domain require
 * the domain to be specified via a *type* `DomainName` passed as an explicit
 * template parameter.
 *
 * The type `Domain::global` may be used to indicate that the global NVTX domain
 * should be used.
 *
 * None of the C++ NVTX constructs require the user to manually construct a
 * `Domain` object. Instead, if a custom domain is desired, the user is expected
 * to define a type `DomainName` that contains a member `DomainName::name` which
 * resolves to either a `char const*` or `wchar_t const*`. The value of
 * `DomainName::name` is used to name and uniquely identify the custom domain.
 *
 * Upon the first use of an NVTX construct associated with the type
 * `DomainName`, the "construct on first use" pattern is used to construct a
 * function local static `Domain` object. All future NVTX constructs associated
 * with `DomainType` will use a reference to the previously constructed `Domain`
 * object. See `Domain::get`.
 *
 * Example:
 * ```c++
 * // The type `my_domain` defines a `name` member used to name and identify the
 * // `Domain` object identified by `my_domain`.
 * struct my_domain{ static constexpr char const* name{"my_domain"}; };
 *
 * // The NVTX range `r` will be grouped with all other NVTX constructs
 * // associated with  `my_domain`.
 * nvtx::domain_thread_range<my_domain> r{};
 *
 * // An alias can be created for a `domain_thread_range` in the custom domain
 * using my_thread_range = nvtx::domain_thread_range<my_domain>;
 * my_thread_range my_range{};
 *
 * // `Domain::global` indicates that the global NVTX domain is used
 * nvtx::domain_thread_range<Domain::global> r2{};
 *
 * // For convenience, `nvtx::thread_range` is an alias for a range in the
 * // global domain
 * nvtx::thread_range r3{};
 * ```
 *---------------------------------------------------------------------------**/
class Domain {
 public:
  Domain(Domain const&) = delete;
  Domain& operator=(Domain const&) = delete;
  Domain(Domain&&) = delete;
  Domain& operator=(Domain&&) = delete;

  /**
   * @brief Returns reference to an instance of a `Domain` constructed using the
   * specified name created as a function local static.
   *
   * None of the constructs in this header require the user to directly invoke
   * `Domain::get`. It is automatically invoked when constructing objects like a
   * `thread_range` or `Category`. Advanced users may wish to use `Domain::get`
   * for the convenience of the "construct on first use" idiom when using
   * domains with their own use of the NVTX C API.
   *
   * Uses the "construct on first use" idiom to safely ensure the `Domain`
   * object is initialized exactly once upon first invocation of
   * `Domain::get<DomainName>()`. All following invocations will return a
   * reference to the previously constructed `Domain` object. See
   * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
   *
   * This function is threadsafe as of C++11. If two or more threads call
   * `Domain::get<DomainName>` concurrently, exactly one of them is guaranteed
   * to construct the `Domain` object and the other(s) will receive a reference
   * to the object after it is fully constructed.
   *
   * The Domain's name is specified via template parameter `DomainName`.
   * The type `DomainName` is required to contain a member `DomainName::name`
   * that resolves to either a `char const*` or `wchar_t const*`. The value of
   * `DomainName::name` is used to name and uniquely identify the `Domain`.
   *
   * Example:
   * ```c++
   * // The type `my_domain` defines a `name` member used to name and identify
   * // the `Domain` object identified by `my_domain`.
   * struct my_domain{ static constexpr char const* name{"my domain"}; };
   *
   * auto D = Domain::get<my_domain>(); // First invocation constructs a
   *                                    // `Domain` with the name "my domain"
   *
   * auto D1 = Domain::get<my_domain>(); // Simply returns reference to
   *                                     // previously constructed `Domain`.
   * ```
   *
   * @tparam DomainName Type that contains a `DomainName::name` member used to
   * name the `Domain` object.
   * @return Reference to the `Domain` created with the specified name.
   */
  template <typename DomainName>
  static Domain const& get() {
    static_assert(detail::has_name_member<DomainName>(),
                  "Type used to identify a Domain must contain a name member of"
                  "type const char* or const wchar_t*");
    static Domain const d{DomainName::name};
    return d;
  }

  /**---------------------------------------------------------------------------*
   * @brief Conversion operator to `nvtxDomainHandle_t`.
   *
   * Allows transparently passing a Domain object into an API expecting a native
   * `nvtxDomainHandle_t` object.
   *---------------------------------------------------------------------------**/
  operator nvtxDomainHandle_t() const noexcept { return _domain; }

  /**
   * @brief Tag type for the "global" NVTX domain.
   *
   * This type may be passed as a template argument to any function/class
   * expecting a `Domain` argument to indicate that the global domain should be
   * used.
   *
   * All NVTX events in the global domain across all libraries and applications
   * will be grouped together.
   *
   */
  struct global {};

 private:
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

  /**
   * @brief Default constructor creates a `Domain` representing the
   * "global" NVTX domain.
   *
   * All events not associated with a custom `Domain` are grouped in the
   * "global" NVTX domain.
   *
   */
  Domain() = default;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the Domain object, unregistering and freeing all domain
   * specific resources.
   *---------------------------------------------------------------------------**/
  ~Domain() noexcept { nvtxDomainDestroy(_domain); }

 private:
  nvtxDomainHandle_t const _domain{};  ///< The `Domain`s NVTX handle
};

/**
 * @brief Returns reference to the `Domain` object that represents the global
 * NVTX domain.
 *
 * This specialization for `Domain::global` returns a default constructed,
 * `Domain` object for use when the "global" domain is desired.
 *
 * All NVTX events in the global domain across all libraries and applications
 * will be grouped together.
 *
 */
template <>
Domain const& Domain::get<Domain::global>() {
  static Domain const d{};
  return d;
}

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
   * @param argb The alpha, red, green, blue components of the desired `Color`
   */
  constexpr Color(ARGB argb) noexcept
      : Color{from_bytes_msb_to_lsb(argb.alpha, argb.red, argb.green,
                                    argb.blue)} {}

  /**
   * @brief Construct a `Color` using the red, green, blue components in
   * `rgb`.
   *
   * Uses maximum value for the alpha channel (opacity) of the `Color`.
   *
   * @param rgb The red, green, blue components of the desired `Color`
   */
  constexpr Color(RGB rgb) noexcept
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
 * @brief Construct for intra-domain grouping of NVTX events.
 *
 * A `Category` allows for more fine-grain grouping of NVTX events than a
 * `Domain`. While it is typical for a library to only have a single `Domain`,
 * it may have several `Category`s. For example, one might have separate
 * categories for IO, memory allocation, compute, etc.
 *
 * @tparam Domain Type containing `name` member used to identify the `Domain` to
 * which the `Category` belongs. Else, `Domain::global` to  indicate that the
 * global NVTX domain should be used.
 *---------------------------------------------------------------------------**/
template <typename D = Domain::global>
class Category {
 public:
  using id_type = uint32_t;
  /**
   * @brief Construct a `Category` with the specified `id`.
   *
   * The `Category` will be unnamed and identified only by it's `id` value.
   *
   */
  constexpr explicit Category(id_type id) noexcept : id_{id} {}

  /**
   * @brief Construct a `Category` with the specified `id` and `name`.
   *
   * The name `name` will be registered with `id`.
   *
   */
  constexpr Category(id_type id, const char* name) noexcept : id_{id} {
    nvtxDomainNameCategoryA(Domain::get<D>(), get_id(), name);
  };

  /**
   * @brief Construct a `Category` with the specified `id` and `name`.
   *
   * The name `name` will be registered with `id`.
   *
   */
  constexpr Category(id_type id, const wchar_t* name) noexcept : id_{id} {
    nvtxDomainNameCategoryW(Domain::get<D>(), get_id(), name);
  };

  /**
   * @brief Returns the id of the Category.
   *
   */
  constexpr id_type get_id() const noexcept { return id_; }

  Category() = delete;
  ~Category() = default;
  Category(Category const&) = delete;
  Category& operator=(Category const&) = delete;
  Category(Category&&) = delete;
  Category& operator=(Category&&) = delete;

 private:
  id_type const id_{};  ///< Category's unique identifier
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
 * which the `Category` belongs. Else, `Domain::global` to  indicate that the
 * global NVTX domain should be used.
 */
template <typename Name, uint32_t Id, typename D = Domain::global>
Category<D> const& get_category() noexcept {
  static_assert(detail::has_name_member<Name>(),
                "Type used to name a Category must contain a name member of "
                "type const char* or const wchar_t*");
  static Category<D> const category{Id, Name::name};
  return category;
}

/**
 * @brief A message registered with NVTX.
 *
 * Message registration is an optimization to lower the overhead of associating
 * a message with an NVTX event by avoiding copying the contents of the message
 * for each event.
 *
 * Registering a message yields a handle that may be used with any NVTX event.
 *
 * @tparam Domain Type containing `name` member used to identify the `Domain` to
 * which the `RegisteredMessage` belongs. Else, `Domain::global` to  indicate
 * that the global NVTX domain should be used.
 */
template <typename D = Domain::global>
class RegisteredMessage {
 public:
  constexpr explicit RegisteredMessage(char const* msg) noexcept
      : value_{nvtxDomainRegisterStringA(Domain::get<D>(), msg)} {}

  constexpr explicit RegisteredMessage(wchar_t const* msg) noexcept
      : value_{nvtxDomainRegisterStringW(Domain::get<D>(), msg)} {}

  constexpr nvtxStringHandle_t get_handle() const noexcept { return value_; }

 private:
  nvtxStringHandle_t const value_{};
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
 * which the `RegisteredMessage` belongs. Else, `Domain::global` to  indicate
 * that the global NVTX domain should be used.
 */
template <typename Message, typename D = Domain::global>
RegisteredMessage<D> const& get_registered_message() noexcept {
  static RegisteredMessage<D> const registered_message{Message::message};
  return registered_message;
}

/**
 * @brief Allows associating a message string with an NVTX event via
 * it's `EventAttribute`s.
 *
 * Example:
 * ```c++
 * // Creates an `EventAttributes` with message "message 0"
 * nvtx::EventAttributes attr0{nvtx::Message{"message 0"}};
 *
 * // `range0` contains message "message 0"
 * nvtx::thread_range range0{attr0};
 *
 * // `std::string` and string literals are implicitly assumed to be
 * // the contents of an `nvtx::Message`
 * // Creates an `EventAttributes` with message "message 1"
 * nvtx::EventAttributes attr1{"message 1"};
 *
 * // `range1` contains message "message 1"
 * nvtx::thread_range range1{attr1};
 *
 * // `range2` contains message "message 2"
 * nvtx::thread_range range2{nvtx::Mesage{"message 2"}};
 *
 * // `std::string` and string literals are implicitly assumed to be
 * // the contents of an `nvtx::Message`
 * // `range3` contains message "message 3"
 * nvtx::thread_range range3{"message 3"};
 * ```
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
   * to which the `RegisteredMessage` belongs. Else, `Domain::global` to
   * indicate that the global NVTX domain should be used.
   * @param msg The message that has already been registered with NVTX.
   */
  template <typename D>
  NVTX_RELAXED_CONSTEXPR Message(RegisteredMessage<D> msg) noexcept
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
 * @brief A numerical value that can be associated with an NVTX event via
 * its `EventAttributes`.
 *
 * Example:
 * ```c++
 * nvtx:: EventAttributes attr{nvtx::Payload{42}}; // Constructs a Payload from
 *                                                 // the `int32_t` value 42
 *
 * // `range0` will have an int32_t payload of 42
 * nvtx::thread_range range0{attr};
 *
 * // range1 has double payload of 3.14
 * nvtx::thread_range range1{ nvtx::Payload{3.14} };
 * ```
 */
class Payload {
 public:
  using value_type = typename nvtxEventAttributes_v2::payload_t;

  /**
   * @brief Construct a `Payload` from a signed, 8 byte integer.
   *
   * @param value Value to use as contents of the payload
   */
  NVTX_RELAXED_CONSTEXPR explicit Payload(int64_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_INT64}, value_{} {
    value_.llValue = value;
  }

  /**
   * @brief Construct a `Payload` from a signed, 4 byte integer.
   *
   * @param value Value to use as contents of the payload
   */
  NVTX_RELAXED_CONSTEXPR explicit Payload(int32_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_INT32}, value_{} {
    value_.iValue = value;
  }

  /**
   * @brief Construct a `Payload` from an unsigned, 8 byte integer.
   *
   * @param value Value to use as contents of the payload
   */
  NVTX_RELAXED_CONSTEXPR explicit Payload(uint64_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_UNSIGNED_INT64}, value_{} {
    value_.ullValue = value;
  }

  /**
   * @brief Construct a `Payload` from an unsigned, 4 byte integer.
   *
   * @param value Value to use as contents of the payload
   */
  NVTX_RELAXED_CONSTEXPR explicit Payload(uint32_t value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_UNSIGNED_INT32}, value_{} {
    value_.uiValue = value;
  }

  /**
   * @brief Construct a `Payload` from a single-precision floating point value.
   *
   * @param value Value to use as contents of the payload
   */
  NVTX_RELAXED_CONSTEXPR explicit Payload(float value) noexcept
      : type_{NVTX_PAYLOAD_TYPE_FLOAT}, value_{} {
    value_.fValue = value;
  }

  /**
   * @brief Construct a `Payload` from a double-precision floating point value.
   *
   * @param value Value to use as contents of the payload
   */
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
  using value_type = nvtxEventAttributes_t;

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

  template <typename D, typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Category<D> const& c,
                                                  Args const&... args) noexcept
      : EventAttributes(args...) {
    _attributes.category = c.get_id().get_value();
  }

  template <typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Color const& c,
                                                  Args const&... args) noexcept
      : EventAttributes(args...) {
    _attributes.color = c.get_value();
    _attributes.colorType = c.get_type();
  }

  template <typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Payload const& p,
                                                  Args const&... args) noexcept
      : EventAttributes(args...) {
    _attributes.payload = p.get_value();
    _attributes.payloadType = p.get_type();
  }

  template <typename... Args>
  NVTX_RELAXED_CONSTEXPR explicit EventAttributes(Message const& m,
                                                  Args const&... args) noexcept
      : EventAttributes(args...) {
    _attributes.message = m.get_value();
    _attributes.messageType = m.get_type();
  }

  ~EventAttributes() = default;
  EventAttributes(EventAttributes const&) = default;
  EventAttributes& operator=(EventAttributes const&) = default;
  EventAttributes(EventAttributes&&) = default;
  EventAttributes& operator=(EventAttributes&&) = default;

  /**
   * @brief Get raw pointer to underlying NVTX attributes object.
   *
   */
  constexpr value_type const* get() const noexcept { return &_attributes; }

 private:
  value_type _attributes{};
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
 * By default, the `Domain::global` is used, which scopes the range to the
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
template <class D = Domain::global>
class domain_thread_range {
 public:
  /**
   * @brief Construct a `domain_thread_range` from the specified
   * `EventAttributes`
   *
   * Example:
   * ```c++
   * EventAttributes attr{"msg", nvtx::RGB{127,255,0}};
   * domain_thread_range<> range{attr}; // Creates a range with message contents
   *                                    // "msg" and green color
   * ```
   *
   * @param attr `EventAttributes` that describes the desired attributes of the
   * range.
   */
  domain_thread_range(EventAttributes const& attr) noexcept {
    nvtxDomainRangePushEx(Domain::get<D>(), attr.get());
  }

  /**
   * @brief Construct a `domain_thread_range`
   *
   */
  template <typename First, typename... Args,
            typename = typename std::enable_if<std::is_same<
                EventAttributes, typename std::decay<First>>::value>>
  domain_thread_range(First const& first, Args const&... args) noexcept
      : domain_thread_range{EventAttributes{first, args...}} {}

  domain_thread_range() = delete;
  domain_thread_range(domain_thread_range const&) = delete;
  domain_thread_range& operator=(domain_thread_range const&) = delete;
  domain_thread_range(domain_thread_range&&) = delete;
  domain_thread_range& operator=(domain_thread_range&&) = delete;

  /**---------------------------------------------------------------------------*
   * @brief Destroy the domain_thread_range, ending the NVTX range event.
   *---------------------------------------------------------------------------**/
  ~domain_thread_range() noexcept { nvtxDomainRangePop(Domain::get<D>()); }
};

/**
 * @brief Convenience alias for a `thread_range` in the global NVTX domain.
 *
 */
using thread_range = domain_thread_range<>;

}  // namespace nvtx

/**
 * @brief Convenience macro for generating a range in the specified `Domain`
 * from the lifetime of a function
 *
 * This macro is useful for generating an NVTX range in `Domain` from
 * the entry point of a function to its exit. It is intended to be the first
 * line of the function.
 *
 * Constructs a static `RegisteredMessage` using the name of the immediately
 * enclosing function returned by `__func__` and constructs a
 * `nvtx::thread_range` using the registered function name as the range's
 * message.
 *
 * Example:
 * ```c++
 * struct my_domain{static constexpr char const* name{"my_domain"};};
 *
 * void foo(...){
 *    NVTX_FUNC_RANGE_IN(my_domain); // Range begins on entry to foo()
 *    // do stuff
 *    ...
 * } // Range ends on return from foo()
 * ```
 *
 * @param[in] D Type containing `name` member used to identify the
 * `Domain` to which the `RegisteredMessage` belongs. Else,
 * `Domain::global` to  indicate that the global NVTX domain should be used.
 */
#define NVTX_FUNC_RANGE_IN(D)                                               \
  static ::nvtx::RegisteredMessage<D> const nvtx_function_name__{__func__}; \
  ::nvtx::domain_thread_range<D> const nvtx_range__{nvtx_function_name__};

/**
 * @brief Convenience macro for generating a range in the global domain from the
 * lifetime of a function.
 *
 * This macro is useful for generating an NVTX range in the global domain from
 * the entry point of a function to its exit. It is intended to be the first
 * line of the function.
 *
 * Constructs a static `RegisteredMessage` using the name of the immediately
 * enclosing function returned by `__func__` and constructs a
 * `nvtx::thread_range` using the registered function name as the range's
 * message.
 *
 * Example:
 * ```c++
 * void foo(...){
 *    NVTX_FUNC_RANGE(); // Range begins on entry to foo()
 *    // do stuff
 *    ...
 * } // Range ends on return from foo()
 * ```
 */
#define NVTX_FUNC_RANGE() NVTX_FUNC_RANGE_IN(::nvtx::Domain::global)
