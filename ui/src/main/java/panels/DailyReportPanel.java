package panels;

import data.TimeData;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;
import packets.RequestDailyReport;
import packets.RequestID;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import java.awt.*;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

public class DailyReportPanel extends JPanel implements Dockable {
    private MainFrame mainFrame;
    @DockingProperty(name = "month", required = true)
    private int month;
    @DockingProperty(name = "day", required = true)
    private int day;
    @DockingProperty(name = "year", required = true)
    private int year;

    private DailyReportMessage.DailyReport report = null;

    private String persistentID;
    private String titleText;
    private String tabText;

    class Row {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        double hours;
    }

    class TableModel extends AbstractTableModel {
        List<Row> rows = new ArrayList<>();

        @Override
        public int getRowCount() {
            return rows.size();
        }

        @Override
        public int getColumnCount() {
            return 3;
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Category";
                case 1:
                    return "Code";
                case 2:
                    return "Hours";
            }
            return null;
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            if (columnIndex == 2) {
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
            return row.hours;
        }
    }

    JLabel date = new JLabel();

    TableModel model = new TableModel();

    public DailyReportPanel(MainFrame mainFrame, LocalDate date) {
        this.mainFrame = mainFrame;
        month = date.getMonthValue();
        day = date.getDayOfMonth();
        year = date.getYear();

        persistentID = String.format("daily-report-%d-%d-%d", month, day, year);
        titleText = String.format("Daily Report (%d/%d/%d)", month, day, year);
        tabText = String.format("Daily Report (%d/%d/%d)", month, day, year);

        Docking.registerDockable(this);

        buildUI();
    }

    public DailyReportPanel(DynamicDockableParameters parameters) {
        persistentID = parameters.getPersistentID();
        titleText = parameters.getTitleText();
        tabText = parameters.getTabText();

        Docking.registerDockable(this);

        buildUI();
    }

    @Override
    public void updateProperties() {
        mainFrame = MainFrame.mainFrame;

        RequestDailyReport request = new RequestDailyReport();
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

        add(date, gbc);
        gbc.gridy++;

        JLabel start = new JLabel();
        JLabel end = new JLabel();
        JLabel total = new JLabel();

        add(start, gbc);
        gbc.gridy++;
        add(end, gbc);
        gbc.gridy++;
        add(total, gbc);
        gbc.gridy++;

        JTable table = new JTable(model);
        
        add(new JScrollPane(table), gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(DailyReportMessage message) {
        report = message.getReport();

        date.setText(String.format("%d/%d/%d", report.month, report.day, report.year));

        model.rows.clear();
        model.fireTableDataChanged();

        if (report.found) {
            report.timesPerCode.forEach((timeCode, time) -> {
                long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

                minutes = Math.round(minutes / 15.0) * 15;

                if (time.toEpochMilli() != 0 && minutes == 0) {
                    minutes = 15;
                }

                Row row = new Row();
                row.category = mainFrame.getTimeData().timeCategoryForTimeCode(timeCode);
                if (row.category != null) {
                    Optional<TimeData.TimeCode> first = row.category.timeCodes.stream().filter(timeCode1 -> timeCode1.id == timeCode).findFirst();
                    if (first.isPresent()) {
                        row.code = first.get();
                    } else {
                        row.code = new TimeData.TimeCode();
                        row.code.id = 0;
                    }
                }
                else {
                    row.category = new TimeData.TimeCategory();
                    row.category.name = "Unknown";
                    row.code = new TimeData.TimeCode();
                    row.code.id = 0;
                    row.code.name = "Unknown";
                }
                row.hours = minutes / 60.0;
                model.rows.add(row);
                model.fireTableRowsInserted(model.rows.size() - 1, model.rows.size() - 1);
            });
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
}
