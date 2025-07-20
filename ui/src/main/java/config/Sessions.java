package config;

import data.Task;
import data.TimeData;
import packets.TaskInfo;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import java.awt.*;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;

class Sessions extends JPanel {
    class SessionTableModel extends AbstractTableModel {
        public List<TaskInfo.Session> data = new ArrayList<>();

        @Override
        public int getRowCount() {
            return data.size();
        }

        @Override
        public String getColumnName(int column) {
            if (column == 0) {
                return "Start";
            }
            return "Stop";
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return Instant.class;
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            TaskInfo.Session row = data.get(rowIndex);
            if (columnIndex == 0) {
                return row.startTime;
            }
            if (!row.stopTime.isPresent()) {
                return null;
            }
            return row.stopTime.get();
        }
    }

    class TimeEntryTableModel extends AbstractTableModel {
        public List<TimeData.TimeEntry> data = new ArrayList<>();

        @Override
        public int getRowCount() {
            return data.size();
        }

        @Override
        public String getColumnName(int column) {
            if (column == 0) {
                return "Category";
            }
            return "Code";
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return String.class;
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            TimeData.TimeEntry row = data.get(rowIndex);
            if (columnIndex == 0) {
                if (row.category == null) {
                    return null;
                }
                return row.category.name;
            }
            if (row.code == null) {
                return null;
            }
            return row.code.name;
        }
    }

    SessionTableModel sessionModel = new SessionTableModel();
    JTable sessionTable = new JTable(sessionModel);

    TimeEntryTableModel timeEntryModel = new TimeEntryTableModel();
    JTable timeEntryTable = new JTable(timeEntryModel);

    Sessions(Task task) {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
        split.setResizeWeight(0.75);
        split.setLeftComponent(new JScrollPane(sessionTable));
        split.setRightComponent(new JScrollPane(timeEntryTable));

        add(split, gbc);

        sessionTable.setDefaultRenderer(Instant.class, new DefaultTableCellRenderer() {
            @Override
            public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column) {
                JLabel label = (JLabel) super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);

                DateTimeFormatter dateTimeFormatter = DateTimeFormatter.ofPattern("MM/dd/yyyy hh:mm:ss a");

                if (value != null) {
                    label.setText(((Instant) value).atZone(ZoneId.systemDefault()).format(dateTimeFormatter));
                }

                return label;

            }
        });
        // table of sessions, of which has a table of time entries?
        // start time, stop time, time entry
        for (TaskInfo.Session session : task.sessions) {
            sessionModel.data.add(session);
            sessionModel.fireTableRowsInserted(sessionModel.data.size() - 1, sessionModel.data.size() - 1);
        }

        sessionTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        sessionTable.getSelectionModel().addListSelectionListener(e -> {
            timeEntryModel.data.clear();
            timeEntryModel.fireTableDataChanged();

            if (sessionTable.getSelectedRow() != -1) {
                TaskInfo.Session session = sessionModel.data.get(sessionTable.getSelectedRow());

                for (TimeData.TimeEntry entry : session.timeEntry) {
                    timeEntryModel.data.add(entry);
                    timeEntryModel.fireTableRowsInserted(timeEntryModel.data.size() - 1, timeEntryModel.data.size() - 1);
                }
            }
        });
    }
}
