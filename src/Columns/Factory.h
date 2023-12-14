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

#ifndef COLUMNS_FACTORY_H
#define COLUMNS_FACTORY_H

#include <functional>
#include <memory>

#include <QPointer>

class AbstractColumn;
class DwarfFortressData;
class QJsonObject;
namespace DFHack { class Client; }

namespace Columns {

using Factory = std::function<std::unique_ptr<AbstractColumn>(DwarfFortressData &, QPointer<DFHack::Client>)>;

Factory makeFactory(const QJsonObject &);

}

#endif
