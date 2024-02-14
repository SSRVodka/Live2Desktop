/**
 * @file eventHandler.h
 * @brief This file defines the classes that handle keyboard and mouse events.
 * 
 * `TouchManager` -> Mouse Events
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

class TouchManager {
public:

    TouchManager();

    float GetCenterX() const { return _lastX; }
    float GetCenterY() const { return _lastY; }
    float GetDeltaX() const { return _deltaX; }
    float GetDeltaY() const{ return _deltaY; }
    float GetStartX() const{ return _startX; }
    float GetStartY() const{ return _startY; }
    float GetScale() const { return _scale; }
    float GetX() const{ return _lastX; }
    float GetY() const{ return _lastY; }
    float GetX1() const{ return _lastX1; }
    float GetY1() const{ return _lastY1; }
    float GetX2() const{ return _lastX2; }
    float GetY2() const{ return _lastY2; }
    bool IsSingleTouch() const { return _touchSingle; }
    bool IsFlickAvailable() const { return _flipAvailable; }
    void DisableFlick() { _flipAvailable = false; }

    /**
     * @brief Event at start of touch.
     *
     * @param[in] deviceX    x value of touched screen
     * @param[in] devicey    y value of touched screen
     */
    void TouchesBegan(float deviceX, float deviceY);

    /**
     * @brief Event at drag.
     *
     * @param[in] deviceX    x value of touched screen
     * @param[in] deviceY    y value of touched screen
     */
    void TouchesMoved(float deviceX, float deviceY);

    /**
     * @brief Event at drag.
     *
     * @param[in] deviceX1   x value of the first touched screen
     * @param[in] deviceY1   y value of the first touched screen
     * @param[in] deviceX2   x value of the second touched screen
     * @param[in] deviceY2   y value of the second touched screen
     */
    void TouchesMoved(float deviceX1, float deviceY1, float deviceX2, float deviceY2);

    /**
     * @brief Flick distance measurement.
     *
     * @return Flick Distance
     */
    float GetFlickDistance() const;

private:
    /**
     * @brief Find the distance from point 1 to point 2.
     *
     * @param[in] x1 x value of the first touched screen
     * @param[in] y1 y value of the first touched screen
     * @param[in] x2 x value of the second touched screen
     * @param[in] y2 y value of the second touched screen
     * @return   Distance between two points
     */
    float CalculateDistance(float x1, float y1, float x2, float y2) const;

    /**
     * @brief Find the amount of movement from the two values.
     * 
     * If the directions are different, the amount of movement is 0.
     * If the directions are the same,
     * the value with the smaller absolute value is referenced.
     *
     * @param[in] v1    1st Movement amount
     * @param[in] v2    2nd Movement amount
     *
     * @return   Movement of the smaller
     */
    float CalculateMovingAmount(float v1, float v2);

    float _startX;              /**< Value of x at the start of the touch */
    float _startY;              /**< Value of y at the start of the touch */
    float _lastX;               /**< Value of x at single touch */
    float _lastY;               /**< Value of y at single touch */
    float _lastX1;              /**< Value of the first x at double touch */
    float _lastY1;              /**< Value of the first y at double touch */
    float _lastX2;              /**< Value of the second x at double touch */
    float _lastY2;              /**< Value of the second y at double touch */
    float _lastTouchDistance;   /**< Distance between fingers when touching with two or more fingers */
    float _deltaX;              /**< Distance traveled by x from the previous value to the current value */
    float _deltaY;              /**< Distance traveled by y from the previous value to the current value */
    float _scale;               /**< The magnification ratio to be multiplied in this frame.
                                     1 except during the magnification operation. */
    bool _touchSingle;          /**< True for single touch */
    bool _flipAvailable;        /**< Whether the flip is valid or not */

};
