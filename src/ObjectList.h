/*
 * Copyright 2023 Clement Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef OBJECT_LIST_H
#define OBJECT_LIST_H

#include <QAbstractListModel>

#include <ranges>

class ObjectListBase: public QAbstractListModel
{
	Q_OBJECT
public:
	ObjectListBase(QObject *parent = nullptr);
	~ObjectListBase() override;

signals:
	void unitDataChanged(int row, int unid_id);
};

template <typename T>
concept UpdatableObject = requires (T object, std::unique_ptr<typename T::df_type> df_object) {
	object.update(std::move(df_object));
	QObject::connect(&object, &T::updated, [](){});
};

template <typename T>
concept SortedByKey = requires (T object, typename T::df_type df_object) {
	requires std::is_member_pointer_v<decltype(T::sorted_key)>;
	{ *object } -> std::same_as<const typename T::df_type &>;
	{ df_object.*T::sorted_key } -> std::totally_ordered;
};
template <typename T>
concept NamedObject = requires (T object, typename T::df_type df_object) {
	requires std::is_member_pointer_v<decltype(T::name_member)>;
	{ *object } -> std::same_as<const typename T::df_type &>;
	{ df_object.*T::name_member } -> std::equality_comparable;
};

template <typename T>
concept HasUnitDataChangedSignal = requires (T object) {
	QObject::connect(&object, &T::unitDataChanged, [](int unit_id){});
};

template <SortedByKey T>
struct GetKey {
	using df_type = T::df_type;
	static inline constexpr auto sorted_key = T::sorted_key;
	auto operator()(const T &object) const {
		return (*object).*sorted_key;
	}
	auto operator()(const std::shared_ptr<T> &ptr) const {
		return (**ptr).*sorted_key;
	}
	auto operator()(const std::unique_ptr<df_type> &ptr) const {
		return (*ptr).*sorted_key;
	}
};

template <NamedObject T>
struct GetName {
	using df_type = T::df_type;
	static inline constexpr auto name_member = T::name_member;
	decltype(auto) operator()(const T &object) const {
		return (*object).*name_member;
	}
	decltype(auto) operator()(const std::shared_ptr<T> &ptr) const {
		return (**ptr).*name_member;
	}
	decltype(auto) operator()(const std::unique_ptr<df_type> &ptr) const {
		return (*ptr).*name_member;
	}
};

template <typename F, typename T>
concept ObjectFactory = requires (F factory, std::unique_ptr<typename T::df_type> df_object) {
	{ factory(std::move(df_object)) } -> std::same_as<std::shared_ptr<T>>;
};

template <typename T>
class ObjectList: public ObjectListBase
{
	static_assert(UpdatableObject<T>);
public:
	ObjectList(QObject *parent = nullptr):
		ObjectListBase(parent)
	{
	}
	~ObjectList() override = default;

	using df_type = T::df_type;

	template <ObjectFactory<T> Factory> requires SortedByKey<T>
	void update(std::vector<std::unique_ptr<df_type>> &&new_objects, Factory &&factory)
	{
		static const GetKey<T> get_key;
		auto old_it = _objects.begin();
		auto new_it = new_objects.begin();
		while (old_it != _objects.end() || new_it != new_objects.end()) {
			// Update common units
			auto [old_match_end, new_match_end] = std::ranges::mismatch(
					old_it, _objects.end(),
					new_it, new_objects.end(),
					std::equal_to{}, get_key, get_key);
			if (old_match_end != old_it) {
				int first_row = distance(_objects.begin(), old_it);
				for (auto &u: std::ranges::subrange(new_it, new_match_end))
					(*old_it++)->update(std::move(u));
				new_it = new_match_end;
				int last_row = distance(_objects.begin(), old_it)-1;
				dataChanged(index(first_row), index(last_row));
			}

			// Remove missing units
			auto old_remove_end = new_it == new_objects.end()
				? _objects.end()
				: std::ranges::lower_bound(old_it, _objects.end(),
						get_key(*new_it), std::less{}, get_key);
			if (old_remove_end != old_it) {
				old_it = removeObjects(old_it, old_remove_end);
				continue;
			}

			// Insert new units
			auto new_insert_end = old_it == _objects.end()
				? new_objects.end()
				: std::ranges::lower_bound(new_it, new_objects.end(),
						get_key(*old_it), std::less{}, get_key);
			if (new_insert_end != new_it) {
				old_it = insertNewObjects(
						old_it,
						std::ranges::subrange(new_it, new_insert_end),
						factory);
				new_it = new_insert_end;
			}

		}
	}

	template <ObjectFactory<T> Factory> requires NamedObject<T>
	void update(std::vector<std::unique_ptr<df_type>> &&new_objects, Factory &&factory)
	{
		static const GetName<T> get_name;
		// longest common subsequence for each prefix pair
		auto lcs = std::vector<std::size_t>((_objects.size()+1)*(new_objects.size()+1), 0);
		std::size_t stride = _objects.size()+1;
		for (std::size_t old_i = 0; old_i < _objects.size(); ++old_i)
			for (std::size_t new_i = 0; new_i < new_objects.size(); ++new_i) {
				if (get_name(_objects[old_i]) == get_name(new_objects[new_i]))
					lcs[stride * (new_i+1) + old_i+1] =
						lcs[stride * new_i + old_i] + 1;
				else
					lcs[stride * (new_i+1) + old_i+1] = std::max(
						lcs[stride * (new_i+1) + old_i],
						lcs[stride * new_i + old_i+1]);
			}

		std::size_t old_i = _objects.size(), new_i = new_objects.size();
		while (old_i > 0 || new_i > 0) {
			std::size_t count = 0;
			while (count < new_i && lcs[stride*new_i+old_i] == lcs[stride*(new_i-1-count)+old_i])
				++count;
			if (count > 0) {
				insertNewObjects(
						next(_objects.begin(), old_i),
						std::views::counted(
							next(new_objects.begin(), new_i-count),
							count),
						factory);
				new_i -= count;
				continue;
			}
			while (count < old_i && lcs[stride*new_i+old_i] == lcs[stride*new_i+old_i-1-count])
				++count;
			if (count > 0) {
				removeObjects(next(_objects.begin(), old_i-count),
						next(_objects.begin(), old_i));
				old_i -= count;
				continue;
			}
			_objects[--old_i]->update(std::move(new_objects[--new_i]));
			dataChanged(index(old_i), index(old_i));
		}
	}

	void clear()
	{
		beginResetModel();
		_objects.clear();
		endResetModel();
	}

	int rowCount(const QModelIndex &parent = {}) const override
	{
		return _objects.size();
	}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
	{
		return {};
	}

	const T *get(int row) const
	{
		if (row >= 0 && unsigned(row) < _objects.size())
			return _objects[row].get();
		else
			return nullptr;
	}

	T *get(int row)
	{
		if (row >= 0 && unsigned(row) < _objects.size())
			return _objects[row].get();
		else
			return nullptr;
	}

	template <std::totally_ordered_with<std::invoke_result_t<GetKey<T>, T>> Key>
	QModelIndex find(Key key) const requires SortedByKey<T>
	{
		auto it = std::ranges::lower_bound(_objects, key, std::less{}, GetKey<T>{});
		if (it == _objects.end() || GetKey<T>{}(*it) != key)
			return {};
		else
			return index(distance(_objects.begin(), it));
	}

	QModelIndex find(const T &object) const requires SortedByKey<T>
	{
		return find(GetKey<T>{}(object));
	}

	QModelIndex find(const T &object) const requires (!SortedByKey<T>)
	{
		auto it = std::ranges::find(_objects, &object, &std::shared_ptr<T>::get);
		if (it == _objects.end())
			return {};
		else
			return index(distance(_objects.begin(), it));
	}

private:
	std::vector<std::shared_ptr<T>> _objects;

	using object_iterator = decltype(_objects)::iterator;

	template <std::ranges::random_access_range Rng, ObjectFactory<T> Factory>
	auto insertNewObjects(object_iterator it, Rng &&range, Factory &&factory)
	{
		int first_row = distance(_objects.begin(), it);
		int rows_inserted = size(range);
		beginInsertRows({}, first_row, first_row + rows_inserted - 1);
		auto created = range | std::views::transform([&, this](auto &&df_object) {
			auto ptr = factory(std::move(df_object));
			connect(ptr.get(), &T::updated,
				this, [this]() {
					auto obj = qobject_cast<const T *>(sender());
					Q_ASSERT(obj);
					auto idx = find(*obj);
					if (idx.isValid())
						dataChanged(idx, idx);
				});
			if constexpr (HasUnitDataChangedSignal<T>) {
				connect(ptr.get(), &T::unitDataChanged,
					this, [this](int unit_id) {
						auto obj = qobject_cast<const T *>(sender());
						Q_ASSERT(obj);
						auto idx = find(*obj);
						if (idx.isValid())
							unitDataChanged(idx.row(), unit_id);
					});
			}
			return ptr;
		});
		it = _objects.insert(it,
				std::make_move_iterator(created.begin()),
				std::make_move_iterator(created.end()));
		advance(it, rows_inserted);
		endInsertRows();
		return it;
	}

	auto removeObjects(object_iterator remove_begin, object_iterator remove_end)
	{
		beginRemoveRows({},
				distance(_objects.begin(), remove_begin),
				distance(_objects.begin(), remove_end) - 1);
		// shared objects may still exist after being
		// erased from the vector, so the signals are
		// explicitly disconnected
		for (auto &object: std::ranges::subrange(remove_begin, remove_end))
			object->disconnect(this);
		auto it = _objects.erase(remove_begin, remove_end);
		endRemoveRows();
		return it;
	}

};

#endif
