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
#include "Application.h"
#include "PainterSaver.h"
#include "DataRole.h"

static constexpr int ItemMargin = 3;
static constexpr int ItemBorder = 2;

static QColor withAlpha(const QColor &c, int alpha)
{
	return { c.red(), c.green(), c.blue(), alpha };
}

static QColor mix(const QColor &a, const QColor &b)
{
	return {
		(a.red()+b.red())/2,
		(a.green()+b.green())/2,
		(a.blue()+b.blue())/2,
		(a.alpha()+b.alpha())/2
	};
}

static void drawBorder(QPainter *painter, const QRect &rect, const QPen &pen)
{
	PainterSaver ps(*painter);
	painter->setBrush(Qt::NoBrush);
	painter->setPen(pen);
	painter->drawRect(rect);
}

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
			if (item->index.column() == 0)
				break;
			// Panel
			proxy()->drawPrimitive(QStyle::PE_PanelItemViewItem, item, painter);
			// Check indicator
			auto text_role = QPalette::Text;
			if (item->features & QStyleOptionViewItem::HasCheckIndicator) {
				switch (item->checkState) {
				case Qt::Checked:
					painter->fillRect(
							option->rect.adjusted(ItemMargin, ItemMargin, -ItemMargin, -ItemMargin),
							option->palette.text());
					text_role = QPalette::Base;
					break;
				case Qt::PartiallyChecked:
					painter->drawRect(option->rect.adjusted(ItemMargin, ItemMargin, -ItemMargin-1, -ItemMargin-1));

					break;
				default:
					break;
				}
			}

			auto rating_data = item->index.data(DataRole::RatingRole);
			if (!rating_data.isNull()) {
				auto rating = rating_data.toDouble();
				QPalette palette = option->palette;
				if (rating < 0.0)
					palette.setColor(QPalette::Text, Qt::red);
				PainterSaver ps(*painter);
				switch (Application::settings().rating_display_mode()) {
				case RatingDisplay::GrowingBox:
					painter->setPen(Qt::NoPen);
					painter->setBrush(palette.text());

					rating = std::abs(rating);
					if (rating >= 1.0) {
						// Draw diamond
						int size = std::min(option->rect.width(), option->rect.height()) - 2*ItemMargin;
						painter->setRenderHint(QPainter::Antialiasing);
						auto center = option->rect.toRectF().center();
						painter->drawPolygon(QList<QPointF>{
								center + QPointF{0.0, -size/2.0},
								center + QPointF{size/3.0, 0.0},
								center + QPointF{0.0, size/2.0},
								center + QPointF{-size/3.0, 0.0},
								});
					}
					else if (rating >= 0.05) {
						// Draw square proportional to rating
						int size = (std::min(option->rect.width(), option->rect.height()) - 3*ItemMargin) * rating + 0.5;
						painter->drawRect(option->rect.adjusted(
								(option->rect.width()-size+1)/2,
								(option->rect.height()-size+1)/2,
								-(option->rect.width()-size+1)/2,
								-(option->rect.height()-size+1)/2));
					}
					break;
				case RatingDisplay::Text:
					painter->setFont(item->font);
					proxy()->drawItemText(painter, option->rect, Qt::AlignCenter,
							palette, option->state & QStyle::State_Enabled,
							item->text, text_role);
					break;
				}
			}
			else { // Text content
				PainterSaver ps(*painter);
				painter->setFont(item->font);
				proxy()->drawItemText(painter, option->rect, Qt::AlignCenter,
						option->palette, option->state & QStyle::State_Enabled,
						item->text, text_role);
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

void GridViewStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	switch (element) {
	case PE_PanelItemViewItem:
		if (auto item = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
			auto cg = (widget ? widget->isEnabled() : item->state & QStyle::State_Enabled)
				? (item->state & QStyle::State_Active
					? QPalette::Normal
					: QPalette::Inactive)
				: QPalette::Disabled;
			auto highlight = item->palette.color(cg, QPalette::Highlight);
			auto text = item->palette.color(cg, QPalette::WindowText);
			if (item->index.column() == 0) {
				baseStyle()->drawPrimitive(element, option, painter, widget);
			}
			else {
				if (item->backgroundBrush.style() != Qt::NoBrush) {
					painter->fillRect(item->rect, item->backgroundBrush);
				}
				if (item->state & QStyle::State_Selected) {
					// Draw selected highlight
					painter->fillRect(item->rect, withAlpha(highlight, 100));
				}
				// Draw grid
				drawBorder(painter,
						item->rect.adjusted(0, 0, -1, -1),
						QColor(withAlpha(text, 50)));
			}
			if (item->state & QStyle::State_MouseOver) {
				// Draw mouse over highlight for all columns
				PainterSaver ps(*painter);
				painter->fillRect(item->rect, withAlpha(highlight, 50));
				painter->setPen(mix(highlight, text));
				painter->drawLine(item->rect.topLeft(), item->rect.topRight());
				painter->drawLine(item->rect.bottomLeft(), item->rect.bottomRight());
			}
			return;
		}
		[[fallthrough]];
	default:
		baseStyle()->drawPrimitive(element, option, painter, widget);
		break;
	}
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
