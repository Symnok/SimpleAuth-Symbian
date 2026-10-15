# ZXing C++ (from QZXing 2.3, github.com/ftylitak/qzxing @ 84cf448), QR code reader only:
# the other formats, MultiFormatReader and the iconv/bigint dependencies are left out.
# Apache License 2.0, see LICENSE-ZXing.txt.
INCLUDEPATH += $$PWD
# Without iconv, byte-mode text is passed through as is (otpauth links are ASCII / UTF-8).
DEFINES += NO_ICONV

HEADERS += \
    $$PWD/zxing/BarcodeFormat.h \
    $$PWD/zxing/Binarizer.h \
    $$PWD/zxing/BinaryBitmap.h \
    $$PWD/zxing/ChecksumException.h \
    $$PWD/zxing/DecodeHints.h \
    $$PWD/zxing/Exception.h \
    $$PWD/zxing/FormatException.h \
    $$PWD/zxing/IllegalStateException.h \
    $$PWD/zxing/InvertedLuminanceSource.h \
    $$PWD/zxing/LuminanceSource.h \
    $$PWD/zxing/NotFoundException.h \
    $$PWD/zxing/Reader.h \
    $$PWD/zxing/ReaderException.h \
    $$PWD/zxing/Result.h \
    $$PWD/zxing/ResultPoint.h \
    $$PWD/zxing/ResultPointCallback.h \
    $$PWD/zxing/ZXing.h \
    $$PWD/zxing/common/Array.h \
    $$PWD/zxing/common/BitArray.h \
    $$PWD/zxing/common/BitMatrix.h \
    $$PWD/zxing/common/BitSource.h \
    $$PWD/zxing/common/CharacterSetECI.h \
    $$PWD/zxing/common/Counted.h \
    $$PWD/zxing/common/DecoderResult.h \
    $$PWD/zxing/common/DetectorResult.h \
    $$PWD/zxing/common/GlobalHistogramBinarizer.h \
    $$PWD/zxing/common/GreyscaleLuminanceSource.h \
    $$PWD/zxing/common/GreyscaleRotatedLuminanceSource.h \
    $$PWD/zxing/common/GridSampler.h \
    $$PWD/zxing/common/HybridBinarizer.h \
    $$PWD/zxing/common/IllegalArgumentException.h \
    $$PWD/zxing/common/PerspectiveTransform.h \
    $$PWD/zxing/common/Point.h \
    $$PWD/zxing/common/Str.h \
    $$PWD/zxing/common/StringUtils.h \
    $$PWD/zxing/common/detector/JavaMath.h \
    $$PWD/zxing/common/detector/MathUtils.h \
    $$PWD/zxing/common/detector/MonochromeRectangleDetector.h \
    $$PWD/zxing/common/detector/WhiteRectangleDetector.h \
    $$PWD/zxing/common/reedsolomon/GenericGF.h \
    $$PWD/zxing/common/reedsolomon/GenericGFPoly.h \
    $$PWD/zxing/common/reedsolomon/ReedSolomonDecoder.h \
    $$PWD/zxing/common/reedsolomon/ReedSolomonException.h \
    $$PWD/zxing/qrcode/ErrorCorrectionLevel.h \
    $$PWD/zxing/qrcode/FormatInformation.h \
    $$PWD/zxing/qrcode/QRCodeReader.h \
    $$PWD/zxing/qrcode/Version.h \
    $$PWD/zxing/qrcode/decoder/BitMatrixParser.h \
    $$PWD/zxing/qrcode/decoder/DataBlock.h \
    $$PWD/zxing/qrcode/decoder/DataMask.h \
    $$PWD/zxing/qrcode/decoder/DecodedBitStreamParser.h \
    $$PWD/zxing/qrcode/decoder/Decoder.h \
    $$PWD/zxing/qrcode/decoder/Mode.h \
    $$PWD/zxing/qrcode/detector/AlignmentPattern.h \
    $$PWD/zxing/qrcode/detector/AlignmentPatternFinder.h \
    $$PWD/zxing/qrcode/detector/Detector.h \
    $$PWD/zxing/qrcode/detector/FinderPattern.h \
    $$PWD/zxing/qrcode/detector/FinderPatternFinder.h \
    $$PWD/zxing/qrcode/detector/FinderPatternInfo.h

SOURCES += \
    $$PWD/zxing/BarcodeFormat.cpp \
    $$PWD/zxing/Binarizer.cpp \
    $$PWD/zxing/BinaryBitmap.cpp \
    $$PWD/zxing/ChecksumException.cpp \
    $$PWD/zxing/DecodeHints.cpp \
    $$PWD/zxing/Exception.cpp \
    $$PWD/zxing/FormatException.cpp \
    $$PWD/zxing/InvertedLuminanceSource.cpp \
    $$PWD/zxing/LuminanceSource.cpp \
    $$PWD/zxing/Reader.cpp \
    $$PWD/zxing/Result.cpp \
    $$PWD/zxing/ResultPoint.cpp \
    $$PWD/zxing/ResultPointCallback.cpp \
    $$PWD/zxing/common/BitArray.cpp \
    $$PWD/zxing/common/BitMatrix.cpp \
    $$PWD/zxing/common/BitSource.cpp \
    $$PWD/zxing/common/CharacterSetECI.cpp \
    $$PWD/zxing/common/DecoderResult.cpp \
    $$PWD/zxing/common/DetectorResult.cpp \
    $$PWD/zxing/common/GlobalHistogramBinarizer.cpp \
    $$PWD/zxing/common/GreyscaleLuminanceSource.cpp \
    $$PWD/zxing/common/GreyscaleRotatedLuminanceSource.cpp \
    $$PWD/zxing/common/GridSampler.cpp \
    $$PWD/zxing/common/HybridBinarizer.cpp \
    $$PWD/zxing/common/IllegalArgumentException.cpp \
    $$PWD/zxing/common/PerspectiveTransform.cpp \
    $$PWD/zxing/common/Str.cpp \
    $$PWD/zxing/common/StringUtils.cpp \
    $$PWD/zxing/common/detector/MonochromeRectangleDetector.cpp \
    $$PWD/zxing/common/detector/WhiteRectangleDetector.cpp \
    $$PWD/zxing/common/reedsolomon/GenericGF.cpp \
    $$PWD/zxing/common/reedsolomon/GenericGFPoly.cpp \
    $$PWD/zxing/common/reedsolomon/ReedSolomonDecoder.cpp \
    $$PWD/zxing/common/reedsolomon/ReedSolomonException.cpp \
    $$PWD/zxing/qrcode/QRCodeReader.cpp \
    $$PWD/zxing/qrcode/QRErrorCorrectionLevel.cpp \
    $$PWD/zxing/qrcode/QRFormatInformation.cpp \
    $$PWD/zxing/qrcode/QRVersion.cpp \
    $$PWD/zxing/qrcode/decoder/QRBitMatrixParser.cpp \
    $$PWD/zxing/qrcode/decoder/QRDataBlock.cpp \
    $$PWD/zxing/qrcode/decoder/QRDataMask.cpp \
    $$PWD/zxing/qrcode/decoder/QRDecodedBitStreamParser.cpp \
    $$PWD/zxing/qrcode/decoder/QRDecoder.cpp \
    $$PWD/zxing/qrcode/decoder/QRMode.cpp \
    $$PWD/zxing/qrcode/detector/QRAlignmentPattern.cpp \
    $$PWD/zxing/qrcode/detector/QRAlignmentPatternFinder.cpp \
    $$PWD/zxing/qrcode/detector/QRDetector.cpp \
    $$PWD/zxing/qrcode/detector/QRFinderPattern.cpp \
    $$PWD/zxing/qrcode/detector/QRFinderPatternFinder.cpp \
    $$PWD/zxing/qrcode/detector/QRFinderPatternInfo.cpp
