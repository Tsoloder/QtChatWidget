#pragma once

#include <QPixmap>
#include <QPainter>
#include <QSvgRenderer>
#include <QColor>
#include <QString>

// 从 Qt 资源加载 SVG，渲染成 px×px 的 QPixmap，并按 color 整体着色。
// 用于把简约线条 SVG 图标染成当前主题色，保持统一风格。
// 着色采用 SourceIn 合成：保留原图 alpha（含抗锯齿边缘），把所有不透明像素替换为 color。
inline QPixmap svgTintedPixmap(const QString &resPath, int px, const QColor &color)
{
    QSvgRenderer renderer(resPath);
    if (!renderer.isValid())
        return QPixmap();

    QPixmap src(px, px);
    src.fill(Qt::transparent);
    {
        QPainter p(&src);
        renderer.render(&p);
    }

    QPixmap out(px, px);
    out.fill(Qt::transparent);
    {
        QPainter p(&out);
        p.drawPixmap(0, 0, src);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(out.rect(), color);
    }
    return out;
}
