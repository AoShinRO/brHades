// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef UTILILITIES_HPP
#define UTILILITIES_HPP

#include <algorithm>     
#include <limits>       
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <queue>
#include <condition_variable>
#include <future>
#include <config/core.hpp>
#include "cbasetypes.hpp"
#include "random.hpp"

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

// Class used to perform time measurement
class cScopeTimer {
	struct sPimpl; //this is to avoid long compilation time
	std::unique_ptr<sPimpl> aPimpl;
    
	cScopeTimer();
};

int32 levenshtein( const std::string &s1, const std::string &s2 );

namespace brhades {
	namespace util {
		template <typename K, typename V> bool map_exists( std::map<K,V>& map, K key ){
			return map.find( key ) != map.end();
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> V* map_find( std::map<K,V>& map, K key ){
			auto it = map.find( key );

			if( it != map.end() ){
				return &it->second;
			}else{
				return nullptr;
			}
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> std::shared_ptr<V> map_find( std::map<K,std::shared_ptr<V>>& map, K key ){
			auto it = map.find( key );

			if( it != map.end() ){
				return it->second;
			}else{
				return nullptr;
			}
		}

		/**
		 * Get a key-value pair and return the key value
		 * @param map: Map to search through
		 * @param key: Key wanted
		 * @param defaultValue: Value returned if key doesn't exist
		 * @return Key value on success or defaultValue on failure
		 */
		template <typename K, typename V> V map_get(std::map<K, V>& map, K key, V defaultValue) {
			auto it = map.find(key);

			if (it != map.end())
				return it->second;
			else
				return defaultValue;
		}

		/**
		 * Resize a map.
		 * @param map: Map to resize
		 * @param size: Size to set map to
		 */
		template <typename K, typename V, typename S> void map_resize(std::map<K, V> &map, S size) {
			auto it = map.begin();

			std::advance(it, size);

			map.erase(it, map.end());
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Unordered Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> V* umap_find(std::unordered_map<K, V>& map, K key) {
			auto it = map.find(key);

			if (it != map.end())
				return &it->second;
			else
				return nullptr;
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Unordered Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> std::shared_ptr<V> umap_find(std::unordered_map<K, std::shared_ptr<V>>& map, K key) {
			auto it = map.find(key);

			if (it != map.end())
				return it->second;
			else
				return nullptr;
		}

		/**
		 * Get a key-value pair and return the key value
		 * @param map: Unordered Map to search through
		 * @param key: Key wanted
		 * @param defaultValue: Value returned if key doesn't exist
		 * @return Key value on success or defaultValue on failure
		 */
		template <typename K, typename V> V umap_get(std::unordered_map<K, V>& map, K key, V defaultValue) {
			auto it = map.find(key);

			if (it != map.end())
				return it->second;
			else
				return defaultValue;
		}

		/**
		 * Resize an unordered map.
		 * @param map: Unordered map to resize
		 * @param size: Size to set unordered map to
		 */
		template <typename K, typename V, typename S> void umap_resize(std::unordered_map<K, V> &map, S size) {
			map.erase(std::advance(map.begin(), map.min(size, map.size())), map.end());
		}

		/**
		 * Get a random value from the given unordered map
		 * @param map: Unordered Map to search through
		 * @return A random value by reference
		*/
		template <typename K, typename V> V& umap_random( std::unordered_map<K, V>& map ){
			auto it = map.begin();

			std::advance( it, rnd_value<size_t>( 0, map.size() - 1 ) );

			return it->second;
		}

		/**
		 * Get a random value from the given vector
		 * @param vec: Vector to search through
		 * @return A random value by reference
		*/
		template <typename K> K &vector_random(std::vector<K> &vec) {
			auto it = vec.begin();

			std::advance( it, rnd_value<size_t>( 0, vec.size() - 1 ) );

			return *it;
		}

		/**
		 * Get an iterator element
		 * @param vec: Vector to search through
		 * @param value: Value wanted
		 * @return Key value iterator on success or vector end iterator on failure
		 */
		template <typename K, typename V> typename std::vector<K>::iterator vector_get(std::vector<K> &vec, V key) {
			return std::find(vec.begin(), vec.end(), key);
		}

		/**
		 * Determine if a value exists in the vector
		 * @param vec: Vector to search through
		 * @param value: Value wanted
		 * @return True on success or false on failure
		 */
		template <typename K, typename V> bool vector_exists(const std::vector<K> &vec, V value) {
			auto it = std::find(vec.begin(), vec.end(), value);

			if (it != vec.end())
				return true;
			else
				return false;
		}

		/**
		 * Erase an index value from a vector
		 * @param vector: Vector to erase value from
		 * @param index: Index value to remove
		 */
		template <typename K> void erase_at(std::vector<K>& vector, size_t index) {
			if (vector.size() == 1) {
				vector.clear();
				vector.shrink_to_fit();
			} else
				vector.erase(vector.begin() + index);
		}

		/**
		 * Determine if a value exists in the vector and then erase it
		 * This will only erase the first occurrence of the value
		 * @param vector: Vector to erase value from
		 * @param value: Value to remove
		 */
		template <typename K, typename V> void vector_erase_if_exists(std::vector<K> &vector, V value) {
			auto it = std::find(vector.begin(), vector.end(), value);

			if (it != vector.end()) {
				if (vector.size() == 1) {
					vector.clear();
					vector.shrink_to_fit();
				} else
					vector.erase(it);
			}
		}

#if __has_builtin( __builtin_add_overflow ) || ( defined( __GNUC__ ) && !defined( __clang__ ) && defined( GCC_VERSION  ) && GCC_VERSION >= 50100 )
		template <typename T> bool safe_addition(T a, T b, T &result) {
			return __builtin_add_overflow(a, b, &result);
		}
#else
		template <typename T> bool safe_addition( T a, T b, T& result ){
			bool overflow = false;

			if( std::numeric_limits<T>::is_signed ){
				if( b < 0 ){
					if( a < ( (std::numeric_limits<T>::min)() - b ) ){
						overflow = true;
					}
				}else{
					if( a > ( (std::numeric_limits<T>::max)() - b ) ){
						overflow = true;
					}
				}
			}else{
				if( a > ( (std::numeric_limits<T>::max)() - b ) ){
					overflow = true;
				}
			}

			result = a + b;

			return overflow;
		}
#endif

		bool safe_substraction( int64 a, int64 b, int64& result );
		bool safe_multiplication( int64 a, int64 b, int64& result );

		/**
		 * Safely add values without overflowing.
		 * @param a: Holder of value to increment
		 * @param b: Increment by
		 * @param cap: Cap value
		 * @return Result of a + b
		 */
		template <typename T> T safe_addition_cap( T a, T b, T cap ){
			T result;

			if( brhades::util::safe_addition( a, b, result ) ){
				return cap;
			}
			return std::min(result, cap);
		}

		template <typename T> void tolower( T& string ){
			std::transform( string.begin(), string.end(), string.begin(), ::tolower );
		}

		/**
		* Pad string with arbitrary character in-place
		* @param str: String to pad
		* @param padding: Padding character
		* @param num: Maximum length of padding
		*/
		void string_left_pad_inplace(std::string& str, char padding, size_t num);

		/**
		* Pad string with arbitrary character
		* @param original: String to pad
		* @param padding: Padding character
		* @param num: Maximum length of padding
		*
		* @return A copy of original string with padding added
		*/
		std::string string_left_pad(const std::string& original, char padding, size_t num);

		/**
		* Encode base10 number to base62. Originally by lututui
		* @param val: Base10 Number
		* @return Base62 string
		**/
		std::string base62_encode( uint32 val );

		bool ansi_or_utf_check(std::string_view text, uint32 npcid);

		// Cap input to numeric limits of target value and safecast it
		// @param input: any value
		// @return static_cast<decltype(T)> capped min/max value
		template <typename TargetType, typename InputType>
		constexpr forceinline TargetType clamp(InputType input) {
			return static_cast<TargetType>(std::clamp(input, static_cast<InputType>(std::numeric_limits<TargetType>::min()), static_cast<InputType>(std::numeric_limits<TargetType>::max())));	
		}


		class ThreadPool {
		private:
			struct Task {
				std::function<void(void)> func;
				std::promise<void> promise;
				
				Task(std::function<void(void)>&& f, std::promise<void>&& p) : func(std::move(f)), promise(std::move(p)) {}
			};
			
			void workerThread() {
				while (!stop_) {
					std::unique_lock<std::mutex> lock(queue_mutex_);
					cv_.wait(lock, [this]() {
						return !tasks_.empty() || stop_;
						});

					if (stop_)
						return;
					
					try {
						tasks_.front().func();  // Chama a função diretamente
						tasks_.front().promise.set_value(); // Define o valor diretamente
						tasks_.pop(); // Remove a tarefa após a execução
					} catch (...) {
						tasks_.front().promise.set_exception(std::current_exception());
						tasks_.pop(); // Remove a tarefa mesmo se ocorrer uma exceção.
					}
				}
			}
			
			size_t numThreads_;
			std::vector<std::thread> threads_;
			std::queue<Task> tasks_;
			std::mutex queue_mutex_;
			std::condition_variable cv_;
			bool stop_ = false;
#ifndef DETAILED_LOADING_OUTPUT				
			// Novo vetor para armazenar futuros
			std::vector<std::future<void>> futures_;
#endif			
		public:
			ThreadPool(size_t numThreads) : numThreads_(numThreads) {
				threads_.reserve(numThreads_);
				for (size_t i = 0; i < numThreads_; ++i) {
					threads_.emplace_back([this]() { this->workerThread(); });
				}
			}
			
			~ThreadPool() {
				stop_ = true;
				cv_.notify_all();
				for (auto& thread : threads_) {
					thread.join();
				}
			}
#ifndef DETAILED_LOADING_OUTPUT			
			template <typename F, typename... Args>
			auto enqueue(F&& f, Args&&... args) {
				std::promise<void> promise;
				auto future = promise.get_future();
				
				// Adiciona o futuro ao vetor movendo-o
				futures_.emplace_back(std::move(future));
				
				{
					std::unique_lock<std::mutex> lock(queue_mutex_);
					tasks_.push(Task(std::bind(std::forward<F>(f), std::forward<Args>(args)...), std::move(promise)));
				}
				cv_.notify_one();
				return future;
			}
			// Método para esperar todos os futuros
			void waitForAll() {
				for (auto& future : futures_) {
					future.get();  // Espera pelo futuro associado
				}
				futures_.clear();  // Limpa o vetor após esperar
			}
#else
		    template <typename F, typename... Args>
		    auto enqueue(F&& f, Args&&... args) {
		        std::promise<void> promise;
		        auto future = promise.get_future();
		        // Use std::bind to create a callable object suitable for the Task constructor
		        {
		            std::unique_lock<std::mutex> lock(queue_mutex_);
		            tasks_.push(Task(std::bind(std::forward<F>(f), std::forward<Args>(args)...), std::move(promise)));
		        }
		        cv_.notify_one();
		        return future;
		    }
#endif
		}; // Fim da classe ThreadPool

		/// Path node
		struct path_node {
			struct path_node *parent; ///< pointer to parent (for path reconstruction)
			uint16 x; ///< X-coordinate
			uint16 y; ///< Y-coordinate
			unsigned short g_cost; ///< Actual cost from start to this node
			unsigned short f_cost; ///< g_cost + heuristic(this, goal)
			bool flag; ///< SET_OPEN / SET_CLOSED
		};
		class BinaryHeap {
		public:
			std::vector<path_node*> g_openset;

			int calc_index_walkpath(uint16 x,uint16 y) const {
				return (((x)+(y)*MAX_WALKPATH) & (MAX_WALKPATH*MAX_WALKPATH-1));
			}

			int calc_index_naviwalkpath(uint16 x,uint16 y) const {
				return (((x)+(y)*MAX_WALKPATH_NAVI) & (MAX_WALKPATH_NAVI*MAX_WALKPATH_NAVI-1));
			}

			// Returns the length of the heap
			size_t length() const {
				return g_openset.size();
			}
			
			// Returns the capacity of the heap
			size_t capacity() const {
				return g_openset.capacity();
			}
		
			void ensure(size_t n) {
				if (capacity() < n) {
					g_openset.reserve(n); // Reservar espaço adicional, como especificado por 'step'
				}
			}
		
			// Returns the top value of the heap
			path_node* peek() const {
				return g_openset.front();
			}
			
			// Inserts a value in the heap and restores the heap property
			void push(path_node& val) {
				g_openset.push_back(&val);
				size_t i = g_openset.size() - 1;
				siftDown(0, i);
			}
			
			// Removes the top value of the heap and restores the heap property
			void pop() {
				g_openset.front() = g_openset.back();
				g_openset.pop_back();
				if (g_openset.empty()) return;  // Se não restar nenhum elemento, não há mais nada a fazer
				siftUp(0);
			}
			
			// Updates the heap after modifying an element
			void update(size_t idx) {
				siftDown(0, idx);
				siftUp(idx);
			}
			
			// Clears the heap
			void clear() {
				if(g_openset.empty()) return; // Se não tiver nenhum elemento, não há nada a fazer
				g_openset.clear();
			}
		
			/// Pushes path_node to the binary node_heap.
			/// Ensures there is enough space in array to store new element.
			void push_node(path_node* node) {
				push(*node); 
			}
		
			/// Updates path_node in the binary node_heap.
			int update_node(path_node& node)
			{
				auto it = std::find(g_openset.begin(), g_openset.end(), &node);
				if (it == g_openset.end()) {
					return 1;
				}
				size_t i = std::distance(g_openset.begin(), it);
				update(i);
				return 0;
			}
		
			std::function<int(const path_node*, const path_node*)> node_min_to_comp = [](const path_node* i, const path_node* j) {
				return i->f_cost - j->f_cost;
			};
		
			// Helper function to sift down
			void siftDown(size_t startIdx, size_t idx) {
				while (idx > startIdx) {
					size_t parent = (idx - 1) / 2;
					if (node_min_to_comp(g_openset[parent], g_openset[idx]) <= 0) break;
					std::swap(g_openset[parent], g_openset[idx]);
					idx = parent;
				}
			}
		
			// Helper function to sift up
			void siftUp(size_t idx) {
				size_t leftChild = idx * 2 + 1;
				while (leftChild < g_openset.size()) {
					size_t rightChild = idx * 2 + 2;
					size_t swapChild = (rightChild < g_openset.size() && node_min_to_comp(g_openset[leftChild], g_openset[rightChild]) > 0) ? rightChild : leftChild;
					if (node_min_to_comp(g_openset[idx], g_openset[swapChild]) <= 0) break;
					std::swap(g_openset[idx], g_openset[swapChild]);
					idx = swapChild;
					leftChild = idx * 2 + 1;
				}
			}
		};




	}
}

#endif /* UTILILITIES_HPP */
