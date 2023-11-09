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

#ifndef COROUTINE_COUNTER_H
#define COROUTINE_COUNTER_H

#include <QObject>

class Counter: public QObject
{
	Q_OBJECT
public:
	Counter(QObject *parent = nullptr);
	~Counter() override;

	void increment();
	void decrement();

	std::size_t value() const { return _counter; }

signals:
	void zero();

private:
	std::size_t _counter;
};

class CounterGuard
{
public:
	CounterGuard(Counter &counter);
	~CounterGuard();

private:
	Counter &_counter;
};

#endif
