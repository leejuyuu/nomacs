/*******************************************************************************************************
 DkMath.cpp
 Created on:	22.03.2010

 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances

 Copyright (C) 2011-2013 Markus Diem <markus@nomacs.org>
 Copyright (C) 2011-2013 Stefan Fiel <stefan@nomacs.org>
 Copyright (C) 2011-2013 Florian Kleber <florian@nomacs.org>

 This file is part of nomacs.

 nomacs is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 nomacs is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *******************************************************************************************************/

#include "DkMath.h"

#include <QTransform>
#include <cmath>
#include <qassert.h>

namespace nmc
{

// DkRotatingRect --------------------------------------------------------------------
DkRotatingRect::DkRotatingRect(QRectF rect)
{
    if (rect.isEmpty()) {
        for (int idx = 0; idx < 4; idx++)
            mRect.push_back(QPointF());
    } else
        mRect = rect;
}

DkRotatingRect::~DkRotatingRect() = default;

bool DkRotatingRect::isEmpty() const
{
    if (mRect.size() < 4)
        return true;

    QPointF lp = mRect[0];
    for (int idx = 1; idx < mRect.size(); idx++) {
        if (lp != mRect[idx]) {
            return false;
        }
        lp = mRect[idx];
    }
    return true;
}

void DkRotatingRect::setAllCorners(QPointF &p)
{
    for (int idx = 0; idx < mRect.size(); idx++)
        mRect[idx] = p;
}

DkVector DkRotatingRect::getDiagonal(int cIdx) const
{
    DkVector c0 = mRect[cIdx % 4];
    DkVector c2 = mRect[(cIdx + 2) % 4];

    return c2 - c0;
}

QCursor DkRotatingRect::cpCursor(int idx)
{
    double angle = 0;

    if (idx >= 0 && idx < 4) {
        // this seems a bit complicated...
        // however the points are not necessarily stored in clockwise order...
        DkVector e1 = mRect[(idx + 1) % 4] - mRect[idx];
        DkVector e2 = mRect[(idx + 3) % mRect.size()] - mRect[idx];
        e1.normalize();
        e2.normalize();
        DkVector rv = e1 - e2;
        rv = rv.normalVec();
        angle = rv.angle();
    } else {
        DkVector edge = mRect[(idx + 1) % 4] - mRect[idx % 4];
        angle = edge.normalVec().angle(); // the angle of the normal vector
    }

    angle = DkMath::normAngleRad(angle, -CV_PI / 8.0, 7.0 * CV_PI / 8.0);

    if (angle > 5.0 * CV_PI / 8.0)
        return QCursor(Qt::SizeBDiagCursor);
    else if (angle > 3.0 * CV_PI / 8.0)
        return QCursor(Qt::SizeVerCursor);
    else if (angle > CV_PI / 8.0)
        return QCursor(Qt::SizeFDiagCursor);
    else
        return QCursor(Qt::SizeHorCursor);
}

void DkRotatingRect::updateCorner(int cIdx, QPointF nC, DkVector oldDiag)
{
    // index does not exist
    if (cIdx < 0 || cIdx >= mRect.size() * 2)
        return;

    if (mRect[(cIdx + 1) % 4] == mRect[(cIdx + 3) % 4]) {
        QPointF oC = mRect[(cIdx + 2) % 4]; // opposite corner
        mRect[cIdx] = nC;
        mRect[(cIdx + 1) % 4] = QPointF(nC.x(), oC.y());
        mRect[(cIdx + 3) % 4] = QPointF(oC.x(), nC.y());
    }
    // these indices indicate the control points on edges
    else if (cIdx >= 4 && cIdx < 8) {
        DkVector c0 = mRect[cIdx % 4];
        DkVector n = (mRect[(cIdx + 1) % 4] - c0).normalVec();
        n.normalize();

        // compute the offset vector
        DkVector oV = n * n.scalarProduct(nC - c0);

        mRect[cIdx % 4] = (mRect[cIdx % 4] + oV).toQPointF();
        mRect[(cIdx + 1) % 4] = (mRect[(cIdx + 1) % 4] + oV).toQPointF();
    } else {
        // we have to update the n-1 and n+1 corner
        DkVector cN = nC;
        DkVector c0 = mRect[cIdx];
        DkVector c1 = mRect[(cIdx + 1) % 4];
        DkVector c2 = mRect[(cIdx + 2) % 4];
        DkVector c3 = mRect[(cIdx + 3) % 4];

        if (!oldDiag.isEmpty()) {
            DkVector dN = oldDiag.normalVec();
            dN.normalize();

            float d = dN * (cN - c2);
            cN += (dN * -d);
        }

        // new diagonal
        float diagLength = (c2 - cN).norm();
        float diagAngle = (float)(c2 - cN).angle();

        // compute the idx-1 corner
        float c1Angle = (float)(c1 - c0).angle();
        float newLength = cos(c1Angle - diagAngle) * diagLength;
        DkVector nc1 = DkVector((newLength), 0);
        nc1.rotate(-c1Angle);

        // compute the idx-3 corner
        float c3Angle = (float)(c3 - c0).angle();
        newLength = cos(c3Angle - diagAngle) * diagLength;
        DkVector nc3 = DkVector((newLength), 0);
        nc3.rotate(-c3Angle);

        mRect[(cIdx + 1) % 4] = (nc1 + cN).toQPointF();
        mRect[(cIdx + 3) % 4] = (nc3 + cN).toQPointF();
        mRect[cIdx] = cN.toQPointF();
    }
}

const QPolygonF &DkRotatingRect::getPoly() const
{
    return mRect;
}

void DkRotatingRect::setPoly(QPolygonF &poly)
{
    mRect = poly;
}

QPolygonF DkRotatingRect::getClosedPoly()
{
    if (mRect.isEmpty())
        return QPolygonF();

    QPolygonF closedPoly = mRect;
    closedPoly.push_back(closedPoly[0]);

    return closedPoly;
}

QPointF DkRotatingRect::getCenter() const
{
    if (mRect.empty())
        return QPointF();

    DkVector c1 = mRect[0];
    DkVector c2 = mRect[2];

    return ((c2 - c1) * 0.5f + c1).toQPointF();
}

QPointF DkRotatingRect::getTopLeft() const
{
    DkVector v = mRect[0];
    v = v.minVec(mRect[1]);
    v = v.minVec(mRect[2]);
    v = v.minVec(mRect[3]);

    return v.toQPointF();
}

void DkRotatingRect::setSize(const QSizeF &s)
{
    double angle = getAngle() - CV_PI * 0.5;

    QRectF r;
    r.setSize(s);
    r.moveCenter(getCenter());

    mRect = r;

    // assigning a QRectF to a QPolygonF results in a closed polygon - but we want it to be open so remove the last
    // point
    mRect.pop_back();

    rotate(angle);
}

QSize DkRotatingRect::size() const
{
    QPolygonF p = getPoly();

    // default upper left corner is 0
    DkVector xV = DkVector(mRect[3] - mRect[0]).round();
    DkVector yV = DkVector(mRect[1] - mRect[0]).round();

    QPointF s = QPointF(xV.norm(), yV.norm());

    double angle = xV.angle();
    angle = DkMath::normAngleRad(angle, -CV_PI, CV_PI);

    // switch width/height for /\ and \/ quadrants
    if (std::abs(angle) > CV_PI * 0.25 && std::abs(angle) < CV_PI * 0.75) {
        double x = s.x();
        s.setX(s.y());
        s.setY(x);
    }

    return QSize(qRound(s.x()), qRound(s.y()));
}

void DkRotatingRect::setCenter(const QPointF &center)
{
    if (mRect.empty())
        return;

    DkVector diff = getCenter() - center;

    for (int idx = 0; idx < mRect.size(); idx++) {
        mRect[idx] = mRect[idx] - diff.toQPointF();
    }
}

double DkRotatingRect::getAngle() const
{
    // default upper left corner is 0
    DkVector xV = mRect[1] - mRect[0];
    return xV.angle();
}

float DkRotatingRect::getAngleDeg() const
{
    auto sAngle = (float)(getAngle() * DK_RAD2DEG);

    while (sAngle > 90)
        sAngle -= 180;
    while (sAngle < -90)
        sAngle += 180;

    sAngle = qRound(sAngle * 100) / 100.0f; // round to 2 digits

    return sAngle;
}

void DkRotatingRect::getTransform(QTransform &tForm, QPointF &size) const
{
    if (mRect.size() < 4)
        return;

    // default upper left corner is 0
    DkVector xV = DkVector(mRect[3] - mRect[0]).round();
    DkVector yV = DkVector(mRect[1] - mRect[0]).round();

    QPointF ul = QPointF(qRound(mRect[0].x()), qRound(mRect[0].y()));
    size = QPointF(xV.norm(), yV.norm());

    double angle = xV.angle();
    angle = DkMath::normAngleRad(angle, -CV_PI, CV_PI);

    // switch width/height for /\ and \/ quadrants
    if (std::abs(angle) > CV_PI * 0.25 && std::abs(angle) < CV_PI * 0.75) {
        auto x = (float)size.x();
        size.setX(size.y());
        size.setY(x);
    }

    // invariance -> user does not want to make a difference between an upside down mRect
    if (angle > CV_PI * 0.25 && angle < CV_PI * 0.75) {
        angle -= CV_PI * 0.5;
        ul = mRect[1];
    } else if (angle > -CV_PI * 0.75 && angle < -CV_PI * 0.25) {
        angle += CV_PI * 0.5;
        ul = mRect[3];
    } else if (angle >= CV_PI * 0.75 || angle <= -CV_PI * 0.75) {
        angle += CV_PI;
        ul = mRect[2];
    }

    tForm.rotateRadians(-angle);
    tForm.translate(qRound(-ul.x()), qRound(-ul.y())); // round guarantees that pixels are not interpolated
}

QRectF DkRotatingRect::toExifRect(const QSize &size) const
{
    // TODO: if the angle is > 0 I get issues (photoshop has the same interpretation as we do)
    QPointF center = getCenter();

    QPolygonF polygon = getPoly();
    DkVector vec;
    double angle = getAngle();

    for (int i = 0; i < 4; i++) {
        // We need the second quadrant, but I do not know why... just tried it out.
        vec = polygon[i] - center;
        if (vec.x <= 0 && vec.y > 0)
            break;
    }

    vec.rotate(angle * 2);
    vec.abs();

    float top = (float)center.y() - vec.y;
    float bottom = (float)center.y() + vec.y;
    float left = (float)center.x() - vec.x;
    float right = (float)center.x() + vec.x;

    // Normalize the coordinates:
    top /= size.height();
    bottom /= size.height();
    left /= size.width();
    right /= size.width();

    return QRectF(QPointF(left, top), QSizeF(right - left, bottom - top));
}

DkRotatingRect DkRotatingRect::fromExifRect(const QRectF &rect, const QSize &size, double angle)
{
    double a = CV_PI * 0.5 - angle;

    QRectF rt(rect.left() * size.width(),
              rect.top() * size.height(),
              rect.width() * size.width(),
              rect.height() * size.height());

    //
    DkVector ul = rt.topLeft() - rt.center();
    ul.rotate(-a);

    QSizeF s(std::abs(ul.x * 2.0f), std::abs(ul.y * 2.0f));

    QRectF rts(QPoint(), s);
    rts.moveCenter(rt.center());

    DkRotatingRect rr(rts);
    rr.rotate(-a);

    return rr;
}

void DkRotatingRect::transform(const QTransform &translation, const QTransform &rotation)
{
    // apply transform
    QPolygonF p = mRect;
    p = translation.map(p);
    p = rotation.map(p);
    p = translation.inverted().map(p);

    // Check the order or vertexes
    auto signedArea = (float)((p[1].x() - p[0].x()) * (p[2].y() - p[0].y())
                              - (p[1].y() - p[0].y()) * (p[2].x() - p[0].x()));
    // If it's wrong, just change it
    if (signedArea > 0) {
        QPointF tmp = p[1];
        p[1] = p[3];
        p[3] = tmp;
    }

    // update corners
    setPoly(p);
}

void DkRotatingRect::rotate(double angle)
{
    QPointF c = getCenter();

    QTransform tt;
    tt.translate(-c.x(), -c.y());

    QTransform rt;
    rt.rotateRadians(angle - getAngle());

    transform(tt, rt);
}

std::ostream &DkRotatingRect::put(std::ostream &s)
{
    s << "DkRotatingRect: ";
    for (int idx = 0; idx < mRect.size(); idx++) {
        DkVector vec = DkVector(mRect[idx]);
        s << vec << ", ";
    }

    return s;
}

// DkRotatingRectNew --------------------------------------------------------------------
DkRotatingRectNew::DkRotatingRectNew(QRectF rect)
{
    if (rect.isEmpty()) {
        for (int idx = 0; idx < 4; idx++)
            mRectOld.push_back(QPointF());
    } else
        mRectOld = rect;
}

DkRotatingRectNew::~DkRotatingRectNew() = default;

bool DkRotatingRectNew::isEmpty() const
{
    return mRect.width() == 0 || mRect.height() == 0;
}

void DkRotatingRectNew::setAllCorners(const QPointF &p)
{
    mRect = QRectF(p, QSize());
}

DkVector DkRotatingRectNew::getDiagonal(int cIdx) const
{
    DkVector c0 = mRectOld[cIdx % 4];
    DkVector c2 = mRectOld[(cIdx + 2) % 4];

    return c2 - c0;
}

QCursor DkRotatingRectNew::cpCursor(int idx)
{
    double angle = 0;

    if (idx >= 0 && idx < 4) {
        // this seems a bit complicated...
        // however the points are not necessarily stored in clockwise order...
        DkVector e1 = mRectOld[(idx + 1) % 4] - mRectOld[idx];
        DkVector e2 = mRectOld[(idx + 3) % mRectOld.size()] - mRectOld[idx];
        e1.normalize();
        e2.normalize();
        DkVector rv = e1 - e2;
        rv = rv.normalVec();
        angle = rv.angle();
    } else {
        DkVector edge = mRectOld[(idx + 1) % 4] - mRectOld[idx % 4];
        angle = edge.normalVec().angle(); // the angle of the normal vector
    }

    angle = DkMath::normAngleRad(angle, -CV_PI / 8.0, 7.0 * CV_PI / 8.0);

    if (angle > 5.0 * CV_PI / 8.0)
        return QCursor(Qt::SizeBDiagCursor);
    else if (angle > 3.0 * CV_PI / 8.0)
        return QCursor(Qt::SizeVerCursor);
    else if (angle > CV_PI / 8.0)
        return QCursor(Qt::SizeFDiagCursor);
    else
        return QCursor(Qt::SizeHorCursor);
}

void DkRotatingRectNew::updateCorner(int cIdx, QPointF nC, const QSizeF &aspectRatio)
{
    const QPointF newPos = mPointMapInverted.map(nC);
    const bool noAspectRatio = aspectRatio.width() <= 0 || aspectRatio.height() <= 0;
    const QPointF center = mRect.center();
    qDebug() << "updateCorner" << newPos << cIdx << aspectRatio << center;
    switch (cIdx) {
    case 0: {
        if (noAspectRatio) {
            mRect.setTopLeft(newPos);
            break;
        }

        const QPointF vec = newPos - mRect.bottomRight();
        const QSizeF newSize = aspectRatio.scaled(vec.x(), vec.y(), Qt::KeepAspectRatioByExpanding);
        mRect.setLeft(mRect.right() + std::copysign(newSize.width(), vec.x()));
        mRect.setTop(mRect.bottom() + std::copysign(newSize.height(), vec.y()));
        break;
    }
    case 1: {
        if (noAspectRatio) {
            mRect.setTopRight(newPos);
            break;
        }
        const QPointF vec = newPos - mRect.bottomLeft();
        const QSizeF newSize = aspectRatio.scaled(vec.x(), vec.y(), Qt::KeepAspectRatioByExpanding);
        mRect.setRight(mRect.left() + std::copysign(newSize.width(), vec.x()));
        mRect.setTop(mRect.bottom() + std::copysign(newSize.height(), vec.y()));
        break;
    }
    case 2: {
        if (noAspectRatio) {
            mRect.setBottomRight(newPos);
            break;
        }

        const QPointF vec = newPos - mRect.topLeft();
        const QSizeF newSize = aspectRatio.scaled(vec.x(), vec.y(), Qt::KeepAspectRatioByExpanding);
        mRect.setRight(mRect.left() + std::copysign(newSize.width(), vec.x()));
        mRect.setBottom(mRect.top() + std::copysign(newSize.height(), vec.y()));
        break;
    }
    case 3: {
        if (noAspectRatio) {
            mRect.setBottomLeft(newPos);
            break;
        }

        const QPointF vec = newPos - mRect.topRight();
        const QSizeF newSize = aspectRatio.scaled(vec.x(), vec.y(), Qt::KeepAspectRatioByExpanding);
        mRect.setLeft(mRect.right() + std::copysign(newSize.width(), vec.x()));
        mRect.setBottom(mRect.top() + std::copysign(newSize.height(), vec.y()));
        break;
    }
    case 4: {
        mRect.setTop(newPos.y());
        if (noAspectRatio) {
            break;
        }
        mRect.setWidth(std::copysign(mRect.height() / aspectRatio.height() * aspectRatio.width(), mRect.width()));
        mRect.moveLeft(center.x() - mRect.width() / 2);
        qDebug() << center.x() << mRect.center().x();
        break;
    }
    case 5: {
        mRect.setRight(newPos.x());
        if (noAspectRatio) {
            break;
        }
        mRect.setHeight(std::copysign(mRect.width() / aspectRatio.width() * aspectRatio.height(), mRect.height()));
        mRect.moveTop(center.y() - mRect.height() / 2);
        break;
    }
    case 6: {
        mRect.setBottom(newPos.y());
        if (noAspectRatio) {
            break;
        }
        mRect.setWidth(std::copysign(mRect.height() / aspectRatio.height() * aspectRatio.width(), mRect.width()));
        mRect.moveLeft(center.x() - mRect.width() / 2);
        break;
    }
    case 7: {
        mRect.setLeft(newPos.x());
        if (noAspectRatio) {
            break;
        }
        mRect.setHeight(std::copysign(mRect.width() / aspectRatio.width() * aspectRatio.height(), mRect.height()));
        mRect.moveTop(center.y() - mRect.height() / 2);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

QPolygonF DkRotatingRectNew::getPoly() const
{
    QPolygonF p = getClosedPoly();
    p.pop_back();
    return p;
}

void DkRotatingRectNew::setPoly(QPolygonF &poly)
{
    mRectOld = poly;
}

QPolygonF DkRotatingRectNew::getClosedPoly() const
{
    return mPointMap.map(QPolygonF(mRect));
}

QPointF DkRotatingRectNew::getCenter() const
{
    return mRect.center();
}

QPointF DkRotatingRectNew::getTopLeft() const
{
    return mPointMap.map(mRect.topLeft());
}

void DkRotatingRectNew::setSize(const QSizeF &s)
{
    double angle = getAngle() - CV_PI * 0.5;

    QRectF r;
    r.setSize(s);
    r.moveCenter(getCenter());

    mRectOld = r;

    // assigning a QRectF to a QPolygonF results in a closed polygon - but we want it to be open so remove the last
    // point
    mRectOld.pop_back();

    rotate(angle);
}

QSizeF DkRotatingRectNew::size() const
{
    return mRect.normalized().size();
}

void DkRotatingRectNew::setCenter(const QPointF &center)
{
    if (mRectOld.empty())
        return;

    DkVector diff = getCenter() - center;

    for (int idx = 0; idx < mRectOld.size(); idx++) {
        mRectOld[idx] = mRectOld[idx] - diff.toQPointF();
    }
}

float DkRotatingRectNew::getAngleDeg() const
{
    auto sAngle = (float)(getAngle() * DK_RAD2DEG);

    while (sAngle > 90)
        sAngle -= 180;
    while (sAngle < -90)
        sAngle += 180;

    sAngle = qRound(sAngle * 100) / 100.0f; // round to 2 digits

    return sAngle;
}

void DkRotatingRectNew::getTransform(QTransform &tForm, QPointF &size) const
{
    if (mRectOld.size() < 4)
        return;

    // default upper left corner is 0
    DkVector xV = DkVector(mRectOld[3] - mRectOld[0]).round();
    DkVector yV = DkVector(mRectOld[1] - mRectOld[0]).round();

    QPointF ul = QPointF(qRound(mRectOld[0].x()), qRound(mRectOld[0].y()));
    size = QPointF(xV.norm(), yV.norm());

    double angle = xV.angle();
    angle = DkMath::normAngleRad(angle, -CV_PI, CV_PI);

    // switch width/height for /\ and \/ quadrants
    if (std::abs(angle) > CV_PI * 0.25 && std::abs(angle) < CV_PI * 0.75) {
        auto x = (float)size.x();
        size.setX(size.y());
        size.setY(x);
    }

    // invariance -> user does not want to make a difference between an upside down mRectOld
    if (angle > CV_PI * 0.25 && angle < CV_PI * 0.75) {
        angle -= CV_PI * 0.5;
        ul = mRectOld[1];
    } else if (angle > -CV_PI * 0.75 && angle < -CV_PI * 0.25) {
        angle += CV_PI * 0.5;
        ul = mRectOld[3];
    } else if (angle >= CV_PI * 0.75 || angle <= -CV_PI * 0.75) {
        angle += CV_PI;
        ul = mRectOld[2];
    }

    tForm.rotateRadians(-angle);
    tForm.translate(qRound(-ul.x()), qRound(-ul.y())); // round guarantees that pixels are not interpolated
}

QRectF DkRotatingRectNew::toExifRect(const QSize &size) const
{
    // TODO: if the angle is > 0 I get issues (photoshop has the same interpretation as we do)
    QPointF center = getCenter();

    QPolygonF polygon = getPoly();
    DkVector vec;
    double angle = getAngle();

    for (int i = 0; i < 4; i++) {
        // We need the second quadrant, but I do not know why... just tried it out.
        vec = polygon[i] - center;
        if (vec.x <= 0 && vec.y > 0)
            break;
    }

    vec.rotate(angle * 2);
    vec.abs();

    float top = (float)center.y() - vec.y;
    float bottom = (float)center.y() + vec.y;
    float left = (float)center.x() - vec.x;
    float right = (float)center.x() + vec.x;

    // Normalize the coordinates:
    top /= size.height();
    bottom /= size.height();
    left /= size.width();
    right /= size.width();

    return QRectF(QPointF(left, top), QSizeF(right - left, bottom - top));
}

DkRotatingRectNew DkRotatingRectNew::fromExifRect(const QRectF &rect, const QSize &size, double angle)
{
    double a = CV_PI * 0.5 - angle;

    QRectF rt(rect.left() * size.width(),
              rect.top() * size.height(),
              rect.width() * size.width(),
              rect.height() * size.height());

    //
    DkVector ul = rt.topLeft() - rt.center();
    ul.rotate(-a);

    QSizeF s(std::abs(ul.x * 2.0f), std::abs(ul.y * 2.0f));

    QRectF rts(QPoint(), s);
    rts.moveCenter(rt.center());

    DkRotatingRectNew rr(rts);
    rr.rotate(-a);

    return rr;
}

void DkRotatingRectNew::transform(const QTransform &translation, const QTransform &rotation)
{
    // apply transform
    QPolygonF p = mRectOld;
    p = translation.map(p);
    p = rotation.map(p);
    p = translation.inverted().map(p);

    // Check the order or vertexes
    auto signedArea = (float)((p[1].x() - p[0].x()) * (p[2].y() - p[0].y())
                              - (p[1].y() - p[0].y()) * (p[2].x() - p[0].x()));
    // If it's wrong, just change it
    if (signedArea > 0) {
        QPointF tmp = p[1];
        p[1] = p[3];
        p[3] = tmp;
    }

    // update corners
    setPoly(p);
}

void DkRotatingRectNew::rotate(double angle)
{
    QPointF c = getCenter();

    QTransform tt;
    tt.translate(-c.x(), -c.y());

    QTransform rt;
    rt.rotateRadians(angle - getAngle());

    transform(tt, rt);
}

std::ostream &DkRotatingRectNew::put(std::ostream &s)
{
    s << "DkRotatingRectNew: ";
    for (int idx = 0; idx < mRectOld.size(); idx++) {
        DkVector vec = DkVector(mRectOld[idx]);
        s << vec << ", ";
    }

    return s;
}

qreal DkRotatingRectNew::getAngle() const
{
    return mAngle;
}

/**
 * Set the angle in degrees.
 *
 * Set the angle of the rect to the given value.
 * Because a rectangle is rotationally symmetric,
 * the angle will be converted to be in [0, 180).
 * Positive direction is clockwise because the y-axis points downward.
 */
void DkRotatingRectNew::setAngle(qreal angle)
{
    angle = std::fmod(angle, 180);
    if (angle < 0) {
        angle += 180;
    }

    mAngle = angle;

    const QPointF center = getCenter();
    QTransform rot = QTransform().rotate(angle);
    const QPointF translation = (rot.map(-center) + center);
    rot.setMatrix(rot.m11(), rot.m12(), 0, rot.m21(), rot.m22(), 0, translation.x(), translation.y(), 1);
    mPointMap = rot;

    bool invertible;
    mPointMapInverted = rot.inverted(&invertible);
    if (!invertible) {
        qWarning() << "DkRotatingRectNew::setAngle invert mPointMap failed";
    }
}

}
