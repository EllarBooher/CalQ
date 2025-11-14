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

        axisX: x_axis;
        axisY: y_axis;
    }
}
