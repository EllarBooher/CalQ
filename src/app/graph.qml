import QtQuick
import QtGraphs

Item {
    id: mainView
    width: 1280
    height: 720

    GraphsView {
        id: graphsView
        anchors.fill: parent

        theme: GraphsTheme {
            id: graphsTheme
            theme: GraphsTheme.Theme.BlueSeries
            labelBorderVisible: true
            labelBackgroundVisible: true
            backgroundColor: "black"
        }
        seriesList: series

        axisX: ValueAxis {
            id:x_axis

            min: -1
            max: 1
            tickInterval: 1
            subTickCount: 1
            labelDecimals: 3
        }

        axisY: ValueAxis {
            id:y_axis

            min: -1
            max: 1
            tickInterval: 1
            subTickCount: 1
            labelDecimals: 3
        }
    }
}
