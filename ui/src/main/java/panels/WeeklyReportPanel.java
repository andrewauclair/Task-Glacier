package panels;

import data.TimeData;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;
import packets.RequestDailyReport;
import packets.RequestID;
import packets.RequestWeeklyReport;
import packets.WeeklyReport;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import java.awt.*;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import static taskglacier.MainFrame.mainFrame;

public class WeeklyReportPanel extends JPanel implements Dockable {
    private MainFrame mainFrame;
    @DockingProperty(name = "month", required = true)
    private int month;
    @DockingProperty(name = "day", required = true)
    private int day;
    @DockingProperty(name = "year", required = true)
    private int year;

    private String persistentID;
    private String titleText;
    private String tabText;

    class Row {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        Double[] hours = new Double[7];
    }

    class TableModel extends AbstractTableModel {
        List<Row> rows = new ArrayList<>();

        String[] dates = new String[7];

        @Override
        public int getRowCount() {
            return rows.size();
        }

        @Override
        public int getColumnCount() {
            return 9;
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Category";
                case 1:
                    return "Code";
            }
            return dates[column - 2];
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            if (columnIndex > 1) {
                return double.class;
            }
            return String.class;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            Row row = rows.get(rowIndex);

            if (columnIndex == 0) {
                return row.category.name;
            }
            else if (columnIndex == 1) {
                return row.code.name;
            }
            return row.hours[columnIndex - 2];
        }
    }

    private WeeklyReport report = null;

    TableModel model = new TableModel();

    public WeeklyReportPanel(MainFrame mainFrame, LocalDate date) {
        this.mainFrame = mainFrame;
        month = date.getMonthValue();
        day = date.getDayOfMonth();
        year = date.getYear();

        persistentID = String.format("weekly-report-%d-%d-%d", month, day, year);
        titleText = String.format("Weekly Report (%d/%d/%d)", month, day, year);
        tabText = String.format("Weekly Report (%d/%d/%d)", month, day, year);

        Docking.registerDockable(this);

        buildUI();
    }

    public WeeklyReportPanel(DynamicDockableParameters parameters) {
        persistentID = parameters.getPersistentID();
        titleText = parameters.getTitleText();
        tabText = parameters.getTabText();

        Docking.registerDockable(this);

        buildUI();
    }

    @Override
    public void updateProperties() {
        mainFrame = MainFrame.mainFrame;

        RequestWeeklyReport request = new RequestWeeklyReport();
        request.requestID = RequestID.nextRequestID();
        request.month = month;
        request.day = day;
        request.year = year;

        mainFrame.getConnection().sendPacketWhenReady(request);
    }

    private void buildUI() {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;

        JTable table = new JTable(model);
        table.setAutoCreateRowSorter(true);

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        add(new JScrollPane(table), gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(WeeklyReport message) {
        report = message;

        model.rows.clear();
        model.fireTableDataChanged();

        Map<TimeData.TimeEntry, Row> rows = new HashMap<>();

        for (int i = 0; i < 7; i++) {
            DailyReportMessage.DailyReport report = this.report.reports[i];

            model.dates[i] = String.format("%d/%d/%d", report.month, report.day, report.year);

            if (report.found) {
                final int index = i;

                report.timesPerTimeEntry.forEach((timeEntry, time) -> {
                    long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

                    minutes = Math.round(minutes / 15.0) * 15;

                    if (time.toEpochMilli() != 0 && minutes == 0) {
                        minutes = 15;
                    }

                    Row row = rows.getOrDefault(timeEntry, new Row());
                    row.category = timeEntry.category;
                    row.code = timeEntry.code;

                    if (row.code == null) {
                        row.code = new TimeData.TimeCode();
                        row.code.id = 0;
                        row.code.name = "Unknown";
                    }
                    row.hours[index] = minutes / 60.0;
                    rows.put(timeEntry, row);
                });
            }
        }

        model.fireTableStructureChanged();

        for (Row value : rows.values()) {
            model.rows.add(value);
            model.fireTableRowsInserted(model.rows.size() - 1, model.rows.size() - 1);
        }
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTitleText() {
        return titleText;
    }

    @Override
    public String getTabText() {
        return tabText;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }
}
