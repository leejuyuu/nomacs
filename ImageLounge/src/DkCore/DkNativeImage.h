#pragma once
#include <QImage>

#ifdef WITH_OPENCV
#include "opencv2/core/mat.hpp"
#endif

namespace nmc
{
#ifdef WITH_OPENCV

// container for image in a "native" format supported by nomacs, which
// for the moment means it can map directly to/from cv::Mat
/**
 * @brief A wrapper for sharing image buffers between Qt and OpenCV.
 *
 * DkNativeImage represents a shared image buffer that is accessible as a `const QImage&`
 * and exposes a `cv::Mat&` view that can be processed by OpenCV.
 * During construction, the image may be converted to a format that is supported by
 * both QImage and cv::Mat.
 *
 * The exposed references remain valid as long as the lifetime of
 * the DkNativeImage instance.
 */
class DkNativeImage
{
    QImage mImg{};
    cv::Mat mMat{};
    bool mReadOnly = false;

    static int compatibleCvFormat(QImage::Format qtFormat, int options = map_anyrgb);

    DkNativeImage(QImage &&img, cv::Mat &&mat, bool readOnly)
        : mImg(std::move(img))
        , mMat(std::move(mat))
        , mReadOnly{readOnly}
    {
    }

    static DkNativeImage fromImageInner(QImage &&img, int options, bool readOnly);

public:
    DkNativeImage() = default;

    // note: if constructed from cv::Mat, must use .copy() to persist a copy
    [[nodiscard]] const QImage &img() const // return const; any write to mImg will break link with mat
    {
        return mImg;
    }

    /**
     * @brief Provides a mutable view to the underlying image data as OpenCV matrix.
     *
     * The cv::Mat& is valid as long as the DkNativeImage instance.
     * Use cv::Mat::clone() to persist a copy.
     *
     * @warning Accessing this on a read-only image will trigger a fatal error.
     * @return A reference to the cv::Mat for processing.
     */
    cv::Mat &mat()
    {
        if (mReadOnly) {
            qFatal("attempt to access mutable mat of read only DkNativeImage");
        }

        return mMat;
    }

    /**
     * @brief Provides a immutable view to the underlying image data as OpenCV matrix.
     *
     * The const cv::Mat& is valid as long as the DkNativeImage instance.
     * Use cv::Mat::clone() to persist a copy.
     *
     * @return A reference to the cv::Mat for read.
     */
    [[nodiscard]] const cv::Mat &constMat() const
    {
        return mMat;
    }

    /**
     * @brief Returns whether this instance is read-only.
     *
     * When the return value is true, calling `mat()` will trigger a fatal o
     * error.
     *
     * @return Whether this instance is read-only.
     */
    [[nodiscard]] bool readOnly() const
    {
        return mReadOnly;
    }

    // options for converting to native formats
    enum {
        map_bgr = 0x1, // cv::Mat is BGR(A) (or grayscale one channel) -- cv::cvtColor() wants this usually
        map_rgb = 0x2, // cv::Mat is RGB(A) (or grayscale one channel)
        map_anyrgb = map_bgr | map_rgb, // either order acceptable
    };

    /**
     * @brief construct mutable DkNativeImage from QImage, converting if necessary
     *
     * This factory method is used to construct a DkNativeImage and always returns
     * an instance that is mutable via the reference returned from the `mat()` method.
     *
     *
     * This factory method owns the `img` value,
     * however, the image may be converted if the format is not supported by
     * both QImage and cv::Mat, or the data could be copied if image refcount > 1.
     * Therefore, always use the `DkNativeImage::img()` member to get the image
     * after any mutation.
     *
     * The internal image is guarenteed to be detached from any other QImage that share
     * the same buffer. Therefore any mutation will not affect other QImage.
     * (Unless the QImage data is not managed by Qt, please avoid passing those in.)
     *
     * @param img input image It will be moved into the container.
     * @param options map_* options
     * @return A mutable DkNativeImage.
     */
    static DkNativeImage fromImage(QImage &&img, int options = map_anyrgb);

    /**
     * @brief construct DkNativeImage from QImage, converting if necessary
     *
     * This factory method is used to construct a DkNativeImage from a `const QImage&`.
     * Conditioning on whether the image is converted, the resulting DkNativeImage may or may not
     * allow mutation via `mat()`. The caller must check with `readOnly()` before
     * attempting to call `mat()`.
     *
     * If the conversion is not required, this method will not copy the image data.
     * Due to the detach mechanisms of QImage, any modification of the same implicitly shared
     * image outside of this instance will not be reflected.
     * (Unless the QImage data is not managed by Qt, please avoid passing those in.)
     *
     * @param img input image
     * @param options map_* options
     * @return A mutable DkNativeImage.
     */
    static DkNativeImage fromConstImage(const QImage &img, int options = map_anyrgb);

    /**
     * @brief construct from QImage, converting if necessary
     * @param mat input mat, always takes a shallow, non-COW copy (reference counted by cv::Mat)
     * @param srcImg image for restoring/setting the format and colorspace
     * @param options map_* options
     *
     * @note Unlike fromImage() this will not perform any conversions into the non-native
     *       Qt pixel formats. To modify the qimage you must take a deep copy.
     *
     * @note The supported pattern is to use the read-only QImage or copy it out with .copy()
     *
     * @note This will probably go away in favor of fromImage() on pre-allocated QImage,
     *       avoid using it
     *
     * @return see map_* options
     */
    static DkNativeImage fromMat(cv::Mat &mat, const QImage &srcImg, int options = map_bgr);

    /**
     * @brief construct an empty image with the same format and colorspace
     * @param size
     * @return
     */
    DkNativeImage allocateLike(const QSize &size = {}) const;
};

// const-ified DkNativeImage (provides limited footgun protection on cv::Mat)
class DkConstNativeImage : public DkNativeImage
{
private:
    DkConstNativeImage(DkNativeImage &&img)
        : DkNativeImage(std::move(img))
    {
    }

public:
    cv::Mat &mat() = delete;

    /**
     * @brief immutable version
     *
     * @note use this if you will not mutate the cv::Mat, as there is rarely
     *       any copying of the image buffer (for common formats)
     *
     * @warning OpenCV OutputArray parameters will write into const cv::Mat&,
     *          be sure not to use it in this way or otherwise cast away
     *          const.
     */
    static DkConstNativeImage fromImage(const QImage &img, int options = map_anyrgb);
};

#endif // WITH_OPENCV

}
