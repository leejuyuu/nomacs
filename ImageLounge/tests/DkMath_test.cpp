#include "../src/DkCore/DkMath.h"
#include <cmath>
#include <gtest/gtest.h>

TEST(DkRotatingRectNewTest, SetAngle)
{
    auto rect = nmc::DkRotatingRectNew();
    const double initial = rect.getAngle();
    EXPECT_EQ(initial, 0);

    rect.setAngle(10.5);
    const double positive = rect.getAngle();
    EXPECT_EQ(positive, 10.5);

    rect.setAngle(-10.5);
    const double negative = rect.getAngle();
    EXPECT_EQ(negative, 169.5);

    rect.setAngle(200);
    const double positiveOver = rect.getAngle();
    EXPECT_EQ(positiveOver, 20);

    rect.setAngle(400);
    const double positiveOverTwice = rect.getAngle();
    EXPECT_EQ(positiveOverTwice, 40);

    rect.setAngle(-400);
    const double negativeOverTwice = rect.getAngle();
    EXPECT_EQ(negativeOverTwice, 140);

    rect.setAngle(180);
    const double edge = rect.getAngle();
    EXPECT_EQ(edge, 0);
}

TEST(DkRotatingRectNewTest, TopLeft)
{
    auto rect = nmc::DkRotatingRectNew(QRectF(500, 400, 100, 100 * std::sqrt(3)));

    rect.setAngle(60);
    const QPointF topLeft60 = rect.getTopLeft();
    EXPECT_NEAR(topLeft60.x(), 600, 1e-6);
    EXPECT_NEAR(topLeft60.y(), 400, 1e-6);

    rect.setAngle(120);
    const QPointF topLeft120 = rect.getTopLeft();
    EXPECT_NEAR(topLeft120.x(), 650, 1e-6);
    EXPECT_NEAR(topLeft120.y(), 400 + 50 * std::sqrt(3), 1e-6);

    rect.setAngle(60);
    const QPointF topLeft602 = rect.getTopLeft();
    EXPECT_NEAR(topLeft602.x(), 600, 1e-6);
    EXPECT_NEAR(topLeft602.y(), 400, 1e-6);

    const QPointF translation = {100, 100 * std::sqrt(3)};
    const QPointF centerBeforeTranslation = rect.getCenter();
    rect.translate(translation);
    const QPointF expectedTranslatedCenter = centerBeforeTranslation + translation;
    const QPointF translatedCenter = rect.getCenter();
    EXPECT_NEAR(translatedCenter.x(), expectedTranslatedCenter.x(), 1e-6);
    EXPECT_NEAR(translatedCenter.y(), expectedTranslatedCenter.y(), 1e-6);
}

TEST(DkRotatingRectNewTest, RotateSetBottomRight)
{
    auto rect = nmc::DkRotatingRectNew(QRectF(500, 400, 100, 100 * std::sqrt(3)));
    const QPointF bottomRight = rect.bottomRight();
    EXPECT_NEAR(bottomRight.x(), 600, 1e-6);
    EXPECT_NEAR(bottomRight.y(), 400 + 100 * std::sqrt(3), 1e-6);

    rect.setAngle(60);
    const QPointF topLeft60 = rect.getTopLeft();
    EXPECT_NEAR(topLeft60.x(), 600, 1e-6);
    EXPECT_NEAR(topLeft60.y(), 400, 1e-6);

    const QPointF bottomRight60 = rect.bottomRight();
    EXPECT_NEAR(bottomRight60.x(), 500, 1e-6);
    EXPECT_NEAR(bottomRight60.y(), 400 + 100 * std::sqrt(3), 1e-6);

    rect.updateCorner(2, bottomRight, {});
    const QPointF bottomRight60Updated = rect.bottomRight();
    EXPECT_NEAR(bottomRight60Updated.x(), 600, 1e-6);
    EXPECT_NEAR(bottomRight60Updated.y(), 400 + 100 * std::sqrt(3), 1e-6);

    const QPointF topLeft60Updated = rect.getTopLeft();
    EXPECT_NEAR(topLeft60Updated.x(), 600, 1e-6);
    EXPECT_NEAR(topLeft60Updated.y(), 400, 1e-6);

    const QPointF center60Updated = rect.getCenter();
    EXPECT_NEAR(center60Updated.x(), 600, 1e-6);
    EXPECT_NEAR(center60Updated.y(), 400 + 50 * std::sqrt(3), 1e-6);
}
