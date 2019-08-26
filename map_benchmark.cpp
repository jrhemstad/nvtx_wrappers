#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unordered_map>

std::array<std::string, 10> category_names{"one",  "two", "three", "four",
                                           "five", "six", "seven", "eight",
                                           "nine", "ten"};

template <typename Duration = std::chrono::microseconds, typename F,
          typename... Args>
typename Duration::rep time_it(F&& fun, Args&&... args) {
  const auto begin = std::chrono::high_resolution_clock::now();
  std::forward<F>(fun)(std::forward<Args>(args)...);
  const auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<Duration>(end - begin).count();
}

void unordered_map_test(std::vector<int> const& insert_indices) {
  std::unordered_map<std::string, uint32_t> name_to_id;
  uint32_t counter{0};
  for (auto index : insert_indices) {
    auto found = name_to_id.find(category_names[index]);
    if (found == name_to_id.end()) {
      name_to_id.insert({category_names[index], ++counter});
    }
  }
}

void map_test(std::vector<int> const& insert_indices) {
  std::map<std::string, uint32_t> name_to_id;
  uint32_t counter{0};
  for (auto index : insert_indices) {
    auto found = name_to_id.find(category_names[index]);
    if (found == name_to_id.end()) {
      name_to_id.insert({category_names[index], ++counter});
    }
  }
}

int main(void) {
  std::default_random_engine generator;
  std::uniform_int_distribution<int> distribution(0, category_names.size() - 1);

  constexpr int number_of_insertions{10'000};
  std::vector<int> insert_order(number_of_insertions);
  std::generate(
      insert_order.begin(), insert_order.end(),
      [&generator, &distribution]() { return distribution(generator); });

  unordered_map_test(insert_order);
  std::cout << "Unordered Map: " << time_it(unordered_map_test, insert_order)
            << " microseconds\n";

  map_test(insert_order);
  std::cout << "Map: " << time_it(map_test, insert_order) << " microseconds\n";
}