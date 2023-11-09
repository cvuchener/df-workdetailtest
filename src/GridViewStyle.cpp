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

#include "GridViewStyle.h"

#include <QStyleOptionHeader>
#include "PainterSaver.h"
#include "DataRole.h"

static constexpr int ItemMargin = 3;
static constexpr int ItemBorder = 2;

GridViewStyle::GridViewStyle(QStyle *style):
	QProxyStyle(style)
{
}

GridViewStyle::~GridViewStyle()
{
}

static QRect rotated(const QRect &r)
{
	return { r.y(), -r.x()-r.width(), r.height(), r.width() };
}

void GridViewStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element) {
	case CE_Header:
		if (auto header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			if (!isVertical(*header))
				break;
			proxy()->drawControl(CE_HeaderSection, option, painter, widget);
			auto subopt = *header;
			if (header->sortIndicator != QStyleOptionHeader::None) {
				subopt.rect = proxy()->subElementRect(SE_HeaderArrow, option, widget);
				baseStyle()->drawPrimitive(PE_IndicatorHeaderArrow, &subopt, painter, widget);
			}
			subopt.rect = proxy()->subElementRect(SE_HeaderLabel, option, widget);
			proxy()->drawControl(CE_HeaderLabel, &subopt, painter, widget);
			return;
		}
		break;
	case CE_HeaderLabel:
		if (auto header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			if (!isVertical(*header))
				break;
			bool has_icon = !header->icon.isNull();
			bool has_text = !header->text.isNull();
			auto margin = proxy()->pixelMetric(PM_HeaderMargin, header, widget);
			auto icon_size = proxy()->pixelMetric(PM_SmallIconSize, header, widget);
			int top = header->rect.top();
			if (has_icon) {
				auto subopt = *header;
				subopt.rect.setHeight(icon_size);
				subopt.text = {};
				subopt.iconAlignment = Qt::AlignCenter;

				baseStyle()->drawControl(CE_HeaderLabel, &subopt, painter, widget);

				top += icon_size + margin;
			}
			if (has_text) {
				PainterSaver ps(*painter);
				painter->rotate(90);

				auto subopt = *header;
				subopt.rect.setTop(top);
				subopt.rect = rotated(subopt.rect);
				subopt.icon = {};

				baseStyle()->drawControl(CE_HeaderLabel, &subopt, painter, widget);
			}
			return;
		}
		break;
	case CE_ItemViewItem:
		if (auto item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
			if (option->state & QStyle::State_MouseOver) {
				// Draw row highlight
				PainterSaver ps(*painter);
				auto highlight = option->palette.color(QPalette::Highlight);
				auto text = option->palette.color(QPalette::Active, QPalette::WindowText);
				painter->setBrush(QColor(
						highlight.red(),
						highlight.green(),
						highlight.blue(),
						50));
				painter->setPen(Qt::NoPen);
				painter->drawRect(option->rect);
				painter->setPen(QColor(
						(highlight.red()+text.red())/2,
						(highlight.green()+text.green())/2,
						(highlight.blue()+text.blue())/2,
						(highlight.alpha()+text.alpha())/2));
				painter->drawLine(option->rect.topLeft(), option->rect.topRight());
				painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
			}
			if (item->index.column() == 0)
				break;
			// Panel
			proxy()->drawPrimitive(QStyle::PE_PanelItemViewItem, item, painter);
			// Check indicator
			QPalette content_palette = option->palette;
			if (item->features & QStyleOptionViewItem::HasCheckIndicator) {
				if (item->checkState == Qt::Checked) {
					painter->fillRect(
							option->rect.adjusted(ItemMargin, ItemMargin, -ItemMargin, -ItemMargin),
							option->palette.text());
					for (auto group: { QPalette::Disabled, QPalette::Active, QPalette::Inactive }) {
						auto oldbase = option->palette.brush(group, QPalette::Base);
						content_palette.setBrush(group, QPalette::Base, option->palette.brush(group, QPalette::Text));
						content_palette.setBrush(group, QPalette::Text, oldbase);
					}
				}
			}

			{ // Text content
				PainterSaver ps(*painter);
				painter->setFont(item->font);
				proxy()->drawItemText(painter, option->rect, Qt::AlignCenter,
						content_palette, option->state & QStyle::State_Enabled,
						item->text, QPalette::Text);
			}

			// Disabled cells
			if (!option->state.testFlag(QStyle::State_Enabled)) {
				auto disabled = QBrush(Qt::red, Qt::DiagCrossPattern);
				painter->fillRect(option->rect, disabled);
			}

			// Border highlight
			auto border = item->index.data(DataRole::BorderRole);
			if (border.canConvert<QBrush>()) {
				PainterSaver ps(*painter);
				QPen pen;
				pen.setBrush(border.value<QBrush>());
				pen.setWidth(ItemBorder);
				painter->setPen(pen);
				painter->drawRect(option->rect.adjusted(ItemBorder, ItemBorder, -ItemBorder, -ItemBorder));
			}
			return;
		}
	default:
		break;
	}
	baseStyle()->drawControl(element, option, painter, widget);
}

int GridViewStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
	switch (metric) {
	case PM_HeaderDefaultSectionSizeHorizontal:
		return baseStyle()->pixelMetric(PM_HeaderDefaultSectionSizeVertical, option, widget);
	default:
		return baseStyle()->pixelMetric(metric, option, widget);
	}
}

QSize GridViewStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
	switch (type) {
	case CT_HeaderSection:
		if (auto header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			if (!isVertical(*header))
				break;
			bool has_icon = !header->icon.isNull();
			bool has_text = !header->text.isNull();
			bool has_sort_indicator = header->sortIndicator != QStyleOptionHeader::None;
			auto margin = proxy()->pixelMetric(PM_HeaderMargin, header, widget);
			auto icon_size = proxy()->pixelMetric(PM_SmallIconSize, header, widget);
			auto sort_indicator_size = proxy()->pixelMetric(PM_HeaderMarkSize, header, widget);
			auto text_size = has_text ? header->fontMetrics.size(0, header->text) : QSize{};
			int content_width = 0;
			if (has_icon && icon_size > content_width)
				content_width = icon_size;
			if (has_text && text_size.height() > content_width)
				content_width = text_size.height();
			if (has_sort_indicator && sort_indicator_size > content_width)
				content_width = sort_indicator_size;
			return QSize {
				margin + content_width + margin,
				margin
					+ (has_icon ? icon_size + margin : 0)
					+ (has_text ? text_size.width() + margin : 0)
					+ (has_sort_indicator ? sort_indicator_size + margin : 0)
			};
		}
		break;
	case CT_ItemViewItem:
		if (auto viewitem = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
			if (viewitem->index.column() == 0)
				break;
			int icon_size = proxy()->pixelMetric(PM_SmallIconSize, viewitem, widget);
			int size = std::max(viewitem->fontMetrics.height(), icon_size)+2*ItemMargin;
			return {size, size};
		}
		break;
	default:
		break;
	}
	return baseStyle()->sizeFromContents(type, option, size, widget);
}

QRect GridViewStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
	switch (element) {
	case SE_HeaderLabel:
		if (auto header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			if (!isVertical(*header))
				break;
			auto margin = proxy()->pixelMetric(PM_HeaderMargin, option, widget);
			auto sort_indicator_size = proxy()->pixelMetric(PM_HeaderMarkSize, header, widget);
			bool has_sort_indicator = header->sortIndicator != QStyleOptionHeader::None;
			return QRect {
				header->rect.x() + margin,
				header->rect.y() + margin,
				header->rect.width() - 2*margin,
				header->rect.height() - 2*margin - (has_sort_indicator ? sort_indicator_size + margin : 0)
			};
		}
		break;
	case SE_HeaderArrow:
		if (auto header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
			if (!isVertical(*header))
				break;
			auto margin = proxy()->pixelMetric(PM_HeaderMargin, header, widget);
			auto sort_indicator_size = proxy()->pixelMetric(PM_HeaderMarkSize, header, widget);
			return QRect {
				header->rect.x() + margin,
				header->rect.bottom() - margin - sort_indicator_size,
				header->rect.width() - 2*margin,
				sort_indicator_size
			};
		}
		break;
	default:
		break;
	}
	return baseStyle()->subElementRect(element, option, widget);
}


bool GridViewStyle::isVertical(const QStyleOptionHeader &header) const
{
	return header.orientation == Qt::Horizontal && header.section != 0;
}
