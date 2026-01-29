package panels;

import data.TimeData;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;
import packets.RequestID;
import packets.RequestWeeklyReport;
import packets.WeeklyReport;
import taskglacier.MainFrame;
import tree.WeeklyReportTreeTable;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableCellRenderer;
import java.awt.*;
import java.time.DayOfWeek;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import static java.time.temporal.TemporalAdjusters.previous;

public class WeeklyReportPanel extends JPanel implements Dockable {
    TableModel model = new TableModel();
    WeeklyReportTreeTable newTable = new WeeklyReportTreeTable();
    private MainFrame mainFrame;

    private boolean currentWeek = false;

    @DockingProperty(name = "month", required = true)
    private int month;
    @DockingProperty(name = "day", required = true)
    private int day;
    @DockingProperty(name = "year", required = true)
    private int year;
    private String persistentID;
    private String titleText;
    private String tabText;
    private WeeklyReport report = null;

    // represents the current week always
    public WeeklyReportPanel(MainFrame mainFrame) {
        this.mainFrame = mainFrame;

        currentWeek = true;

        LocalDate now = LocalDate.now();

        LocalDate sunday = now.with(previous(DayOfWeek.SUNDAY));

        month = sunday.getMonthValue();
        day = sunday.getDayOfMonth();
        year = sunday.getYear();

        persistentID = "weekly-report-current";
        titleText = "Weekly Report (Current)";
        tabText = "Weekly Report (Current)";

        Docking.registerDockable(this);

        buildUI();
    }

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

    private void refreshDate() {
        if (!currentWeek) {
            return;
        }

        LocalDate now = LocalDate.now();

        LocalDate sunday = now.with(previous(DayOfWeek.SUNDAY));

        month = sunday.getMonthValue();
        day = sunday.getDayOfMonth();
        year = sunday.getYear();
    }

    public int getMonth() {
        refreshDate();
        return month;
    }

    public int getDay() {
        refreshDate();
        return day;
    }

    public int getYear() {
        refreshDate();
        return year;
    }

    @Override
    public void updateProperties() {
        mainFrame = MainFrame.mainFrame;

        refreshDate();

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

        JTable table = new JTable(model) {
            @Override
            public Component prepareRenderer(TableCellRenderer renderer, int row, int column) {
                JLabel label = (JLabel) super.prepareRenderer(renderer, row, column);

                if (row == model.getTotalRowStart() || column == model.getTotalColumnIndex()) {
                    int top = row == model.getTotalRowStart() ? 2 : 0;
                    int left = column == model.getTotalColumnIndex() ? 1 : 0;
                    label.setBorder(BorderFactory.createCompoundBorder(BorderFactory.createMatteBorder(top, left, 0, 0, UIManager.getColor("Component.borderColor")), label.getBorder()));
                }

                return label;
            }
        };

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        add(new JScrollPane(newTable), gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(WeeklyReport message) {
        report = message;

        newTable.update(message);

        model.clear();
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

                    if (row.category == null) {
                        row.category = new TimeData.TimeCategory();
                        row.category.id = 0;
                        row.category.name = "Unknown";
                    }

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
            model.addRow(value);
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

    static class TotalRow {
        TimeData.TimeCategory category;
        Double[] hours = new Double[7];
        double total;
    }

    private static class TableModel extends AbstractTableModel {
        String[] dates = new String[7];
        private List<Row> rows = new ArrayList<>();
        private List<TotalRow> totals = new ArrayList<>();

        public int getTotalRowStart() {
            return rows.size();
        }

        public int getTotalColumnIndex() {
            return 9;
        }

        @Override
        public int getRowCount() {
            return rows.size() + totals.size();
        }

        @Override
        public int getColumnCount() {
            return 10;
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Category";
                case 1:
                    return "Code";
                case 9:
                    return "Total";
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
            if (rowIndex < rows.size()) {
                Row row = rows.get(rowIndex);

                switch (columnIndex) {
                    case 0:
                        return row.category.name;
                    case 1:
                        return row.code.name;
                    case 9:
                        return row.total;
                }
                return row.hours[columnIndex - 2];
            }
            else {
                TotalRow totalRow = totals.get(rowIndex - rows.size());

                switch (columnIndex) {
                    case 0:
                        return totalRow.category.name + " - Total";
                    case 1:
                        return null;
                    case 9:
                        return totalRow.total;
                }
                return totalRow.hours[columnIndex - 2];
            }
        }

        public void clear() {
            rows.clear();
            totals.clear();
        }

        public void addRow(Row value) {
            rows.add(value);

            for (int i = 0; i < 7; i++) {
                if (value.hours[i] != null) {
                    value.total += value.hours[i];
                }
            }
            // sort the rows to display the most time at the top
            rows.sort(Comparator.comparingDouble(o -> ((Row) o).total).reversed());

            Optional<TotalRow> totalOptional = totals.stream()
                    .filter(totalRow -> totalRow.category.id == value.category.id)
                    .findFirst();

            if (totalOptional.isPresent()) {
                for (int i = 0; i < 7; i++) {
                    if (value.hours[i] == null) {
                        continue;
                    }
                    if (totalOptional.get().hours[i] == null) {
                        totalOptional.get().hours[i] = 0.0;
                    }
                    totalOptional.get().hours[i] += value.hours[i];
                }
                totalOptional.get().total += value.total;
            }
            else {
                TotalRow total = new TotalRow();
                total.category = value.category;
                for (int i = 0; i < 7; i++) {
                    if (value.hours[i] == null) {
                        continue;
                    }
                    if (total.hours[i] == null) {
                        total.hours[i] = 0.0;
                    }
                    total.hours[i] += value.hours[i];
                }
                total.total += value.total;
                totals.add(total);
            }

            // sort the rows to display the most time at the top
            totals.sort(Comparator.comparingDouble(o -> ((TotalRow) o).total).reversed());
        }
    }

    class Row {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        Double[] hours = new Double[7];
        double total;
    }
}
