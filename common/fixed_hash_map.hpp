#pragma once

#include <array>
#include <cstddef>
#include <functional>

template <typename Key, typename Value, std::size_t Capacity>
class FixedHashMap
{
public:
    static_assert(Capacity > 0, "Capacity must be greater than zero");

    bool insert(const Key& key, const Value& value)
    {
        std::size_t index = hash(key);
        std::size_t first_deleted = Capacity;

        for (std::size_t probe = 0; probe < Capacity; ++probe)
        {
            Entry& entry = entries_[index];

            if (entry.state == EntryState::Occupied)
            {
                if (entry.key == key)
                {
                    entry.value = value;
                    return true;
                }
            }
            else if (entry.state == EntryState::Deleted)
            {
                if (first_deleted == Capacity)
                {
                    first_deleted = index;
                }
            }
            else
            {
                const std::size_t target =
                    first_deleted != Capacity ? first_deleted : index;

                entries_[target].key = key;
                entries_[target].value = value;
                entries_[target].state = EntryState::Occupied;

                ++size_;
                return true;
            }

            index = nextIndex(index);
        }

        if (first_deleted != Capacity)
        {
            entries_[first_deleted].key = key;
            entries_[first_deleted].value = value;
            entries_[first_deleted].state = EntryState::Occupied;

            ++size_;
            return true;
    }

    return false;
}
    Value* find(const Key& key)
    {
        std::size_t index = hash(key);

        for (std::size_t probe = 0; probe < Capacity; ++probe)
        {
            Entry& entry = entries_[index];

            if (entry.state == EntryState::Empty)
                return nullptr;

            if (entry.state == EntryState::Occupied &&
                entry.key == key)
            {
                return &entry.value;
            }

            index = nextIndex(index);
        }

        return nullptr;
    }

    const Value* find(const Key& key) const
    {
        std::size_t index = hash(key);

        for (std::size_t probe = 0; probe < Capacity; ++probe)
        {
            const Entry& entry = entries_[index];

            if (entry.state == EntryState::Empty)
                return nullptr;

            if (entry.state == EntryState::Occupied &&
                entry.key == key)
            {
                return &entry.value;
            }

            index = nextIndex(index);
        }

        return nullptr;
    }
    bool erase(const Key& key)
    {
        std::size_t index = hash(key);

        for (std::size_t probe = 0; probe < Capacity; ++probe)
        {
            Entry& entry = entries_[index];

            if (entry.state == EntryState::Empty)
                return false;

            if (entry.state == EntryState::Occupied &&
                entry.key == key)
            {
                entry.state = EntryState::Deleted;
                --size_;


            if (entry.state == EntryState::Occupied &&
                entry.key == key)
            {
                entry.state = EntryState::Deleted;
                --size_;

                return true;
            }


                return true;
            }

            index = nextIndex(index);
        }

        return false;
    }
    
    bool contains(const Key& key) const
    {
        return find(key) != nullptr;
    }

    std::size_t size() const
    {
        return size_;
    }

    constexpr std::size_t capacity() const
    {
        return Capacity;
    }

    bool empty() const
    {
        return size_ == 0;
    }

private:
    enum class EntryState : unsigned char
    {
        Empty,
        Occupied,
        Deleted
    };

    struct Entry
    {
        Key key{};
        Value value{};
        EntryState state = EntryState::Empty;
    };

    static std::size_t hash(const Key& key)
    {
        return std::hash<Key>{}(key) % Capacity;
    }

    static constexpr std::size_t nextIndex(std::size_t index)
    {
        return (index + 1) % Capacity;
    }
    void clearDeletedEntries()
    {
        for (Entry& entry : entries_)
        {
            if (entry.state == EntryState::Deleted)
                entry.state = EntryState::Empty;
        }
    }

    std::array<Entry, Capacity> entries_{};
    std::size_t size_ = 0;
};
