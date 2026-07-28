// flat map // 26.07.26 // ZeroK
// will replace std::unordered_map in orderbook for order_map_


#pragma once

#include <cstdint>
#include <cstddef>

namespace zerok {

template <
	typename Key,
	typename Value,
	std::size_t Capacity
>
class FlatMap {

private:

	static constexpr std::size_t  MASK  { Capacity - 1 };

	static_assert( Capacity > 0 && 
		      (Capacity & (Capacity - 1)) == 0,
		      "Capacity must be power of 2.\n" );


	enum class State : std::uint8_t {
		Empty,
		Occupied,
		Deleted
	};

	struct Entry {

		Key    key    {};
		Value  value  {};
		State  state  { State::Empty };
	};

	Entry entries_ [ Capacity ];

	std::size_t  size_  {};


public:

	// bool	     insert ( const Key&, const Value& );
	// Value*       find ( const Key& );
	// bool         erase ( const Key& );
	// void	     clear();
	// bool	     empty() const noexcept;
	// std::size_t  size() const noexcept;
	// Entry*	probe ( const Key& );


	[[ nodiscard ]]
	bool insert ( const Key& key, const Value& value ) {

		auto idx  =  key & MASK;	// current bucket/idx

		for (auto i {0uz}; i < Capacity; ++i) {
		// for (auto _ {Capacity}; _-- > 0;) {
		
			Entry& slot  =  entries_ [ idx ];

			if ( slot.state == State::Empty ||
			     slot.state == State::Deleted ) {

				slot.key    =  key;
				slot.value  =  value;
				slot.state  =  State::Occupied;

				++size_;
				return true;
			}

			// if duplicate key
			if ( slot.state == State::Occupied &&
			     slot.key == key ) 
				return false;

			idx  =  (idx + 1) & MASK;	// advance bucket/idx
		}	// for()

		return false;	// table full
	}

	
	[[ nodiscard ]]
	Value*  find ( const Key& key ) {
		
		auto idx  =  key & MASK;

		for (auto i {0uz}; i < Capacity; ++i) {
			
			Entry& slot = entries_ [ idx ];

			if ( slot.state == State::Empty ) 
				return nullptr;

			if ( slot.state == State::Occupied &&
			     slot.key   == key ) 
				return &slot.value;

			idx  =  (idx + 1) & MASK;
		}

		return nullptr;
	}	

	[[ nodiscard ]]
	const Value*  find ( const Key& key ) const {
		
		auto idx  =  key & MASK;

		for (auto i {0uz}; i < Capacity; ++i) {
			
			const Entry& slot = entries_ [ idx ];

			if ( slot.state == State::Empty ) 
				return nullptr;

			if ( slot.state == State::Occupied &&
			     slot.key   == key ) 
				return &slot.value;

			idx  =  (idx + 1) & MASK;
		}

		return nullptr;
	}


	[[ nodiscard ]]
	bool  erase ( const Key& key ) {
		
		auto idx  =  key & MASK;

		for (auto i {0uz}; i < Capacity; ++i) {
			
			Entry& slot = entries_ [ idx ];

			if ( slot.state == State::Empty )
				return false;
			
			if ( slot.state == State::Occupied &&
			     slot.key   == key ) {

				slot.key    =  Key {};	// reset with value init
				slot.value  =  Value {};
				slot.state  =  State::Deleted;

				--size_;

				return true;
			}

			idx  =  (idx + 1) & MASK;
		}

		return false;
	}



	[[ nodiscard ]]
	void  clear () {
		
		for (auto& entry : entries_) { 
			entry = {};
		}

		size_  =  0;
	}


	[[ nodiscard ]]
	bool  empty () const noexcept {
		
		return size_ == 0;
	}


	[[ nodiscard ]]
	std::size_t size () const noexcept {
		
		return size_;
	}
};

} // namespace zerok

