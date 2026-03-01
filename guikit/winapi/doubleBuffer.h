
struct DoubleBuffer {

    HDC hMemDC = nullptr;
    HBITMAP hMemBmp = nullptr;
    HBITMAP hOldBmp = nullptr;
    SIZE szBuffer{};

    DoubleBuffer() = default;

    ~DoubleBuffer() {
        release();
    }

    auto ensure(HDC hdc, const RECT& rcClient) -> bool {
        int width = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;

        if (szBuffer.cx != width || szBuffer.cy != height) {
            release();
            hMemDC = CreateCompatibleDC(hdc);
            hMemBmp = CreateCompatibleBitmap(hdc, width, height);
            hOldBmp = static_cast<HBITMAP>(SelectObject(hMemDC, hMemBmp));
            szBuffer = { width, height };
        }

        return hMemDC != nullptr && hMemBmp != nullptr;
    }

    auto release() -> void {
        if (hMemDC) {
            SelectObject(hMemDC, hOldBmp);
            DeleteObject(hMemBmp);
            DeleteDC(hMemDC);

            hMemDC = nullptr;
            hMemBmp = nullptr;
            hOldBmp = nullptr;
            szBuffer = { 0, 0 };
        }
    }
};
