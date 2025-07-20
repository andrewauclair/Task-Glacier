package panels;

import data.Task;
import data.TaskModel;
import data.TimeData;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;
import packets.RequestDailyReport;
import packets.RequestID;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableCellRenderer;
import java.awt.*;
import java.time.Instant;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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

        public Row(TimeData.TimeCategory category, TimeData.TimeCode code, double hours) {
            this.category = category;
            this.code = code;
            this.hours = hours;
        }
    }

    class TableModel extends AbstractTableModel {
        private List<Row> rows = new ArrayList<>();
        double totalHours = 0;

        public int getTotalRowStart() {
            return rows.size();
        }

        @Override
        public int getRowCount() {
            if (rows.isEmpty()) {
                return 0;
            }
            return rows.size() + 1;
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
            if (rowIndex < rows.size()) {
                Row row = rows.get(rowIndex);

                if (columnIndex == 0) {
                    return row.category.name;
                }
                else if (columnIndex == 1) {
                    return row.code.name;
                }
                return row.hours;
            }
            else {
                if (columnIndex == 0) {
                    return "Total";
                }
                else if (columnIndex == 2) {
                    return totalHours;
                }
            }
            return null;
        }

        void clear() {
            rows.clear();
            totalHours = 0;
        }

        void add(TimeData.TimeCategory category, TimeData.TimeCode code, double hours) {
            rows.add(new Row(category, code, hours));

            totalHours += hours;

            // sort the rows to display the most time at the top
            rows.sort(Comparator.comparingDouble(o -> ((Row) o).hours));
        }
    }

    class TaskRow {
        Task task;
        double hours;
    }

    class TaskTableModel extends AbstractTableModel {
        List<TaskRow> rows = new ArrayList<>();

        @Override
        public int getRowCount() {
            return rows.size();
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Task";
                case 1:
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
            TaskRow row = rows.get(rowIndex);

            if (columnIndex == 0) {
                TaskModel taskModel = mainFrame.getTaskModel();

                List<String> parents = new ArrayList<>();

                StringBuilder text = new StringBuilder();

                int parentID = row.task.parentID;

                while (parentID != 0) {
                    Task task = taskModel.getTask(parentID);

                    if (task == null) {
                        break;
                    }
                    parents.add(0, task.name);

                    parentID = task.parentID;
                }
                for (String parent : parents) {
                    text.append(parent);
                    text.append(" / ");
                }
                text.append(row.task.name);

                return text.toString();
            }
            return row.hours;
        }
    }

    JLabel date = new JLabel();

    TableModel model = new TableModel();
    TaskTableModel taskModel = new TaskTableModel();

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

        JLabel start = new JLabel();
        JLabel end = new JLabel();
        JLabel total = new JLabel();

        add(start, gbc);
        gbc.gridy++;
        add(end, gbc);
        gbc.gridy++;
        add(total, gbc);
        gbc.gridy++;

        JTable table = new JTable(model) {
            @Override
            public Component prepareRenderer(TableCellRenderer renderer, int row, int column) {
                JLabel label = (JLabel) super.prepareRenderer(renderer, row, column);

                if (row == model.getTotalRowStart()) {
                    label.setBorder(BorderFactory.createCompoundBorder(BorderFactory.createMatteBorder(2, 0, 0, 0, Color.WHITE), label.getBorder()));
                }

                return label;
            }
        };
        table.setAutoCreateRowSorter(true);
        table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        JTable taskTable = new JTable(taskModel);
        taskTable.setAutoCreateRowSorter(true);
        taskTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        table.getSelectionModel().addListSelectionListener(e -> {
            taskModel.rows.clear();
            taskModel.fireTableDataChanged();

            if (table.getSelectedRow() != -1 && table.getSelectedRow() < model.rows.size()) {
                Row row = model.rows.get(table.getSelectedRow());

                Map<Task, Double> taskRows = new HashMap<>();

                for (DailyReportMessage.DailyReport.TimePair time : report.times) {
                    Task task = mainFrame.getTaskModel().getTask(time.taskID);
                    TaskInfo.Session session = task.sessions.get(time.index);

                    for (TimeData.TimeEntry timeEntry : session.timeEntry) {
                        if (timeEntry.category.equals(row.category) && timeEntry.code.equals(row.code)) {
                            double hours = taskRows.getOrDefault(task, 0.0);

                            Instant stopTime = session.stopTime.orElseGet(() -> report.time);

                            Instant instant = stopTime.minusMillis(session.startTime.toEpochMilli());

                            long minutes = TimeUnit.MILLISECONDS.toMinutes(instant.toEpochMilli());

//                            minutes = Math.round(minutes / 15.0) * 15;
//
//                            if (instant.toEpochMilli() != 0 && minutes == 0) {
//                                minutes = 15;
//                            }

                            hours += minutes / 60.0;

                            taskRows.put(task, hours);
                        }
                    }
                }

                taskRows.forEach((task, aDouble) -> {
                    TaskRow taskRow = new TaskRow();
                    taskRow.task = task;
                    taskRow.hours = aDouble;
                    taskModel.rows.add(taskRow);
                    taskModel.fireTableRowsInserted(taskModel.rows.size() - 1, taskModel.rows.size() - 1);
                });
            }
        });

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
        split.setTopComponent(new JScrollPane(table));
        split.setBottomComponent(new JScrollPane(taskTable));

        add(split, gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(DailyReportMessage message) {
        report = message.getReport();

        date.setText(String.format("%d/%d/%d", report.month, report.day, report.year));

        model.clear();
        model.fireTableDataChanged();

        if (report.found) {
            report.timesPerTimeEntry.forEach((timeEntry, time) -> {
                long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

                minutes = Math.round(minutes / 15.0) * 15;

                if (time.toEpochMilli() != 0 && minutes == 0) {
                    minutes = 15;
                }

                TimeData.TimeCategory category = timeEntry.category;

                if (category == null) {
                    category = new TimeData.TimeCategory();
                    category.id = 0;
                    category.name = "Unknown";
                }

                TimeData.TimeCode code = timeEntry.code;

                if (code == null) {
                    code = new TimeData.TimeCode();
                    code.id = 0;
                    code.name = "Unknown";
                }

                model.add(category, code, minutes / 60.0);
            });
        }

        model.fireTableDataChanged();
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
