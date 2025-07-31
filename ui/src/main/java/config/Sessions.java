package config;

import data.Task;
import data.TimeData;
import packets.TaskInfo;
import packets.UpdateTask;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import java.awt.*;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import static taskglacier.MainFrame.mainFrame;

class Sessions extends JPanel {
    SessionTableModel sessionModel = new SessionTableModel();
    JTable sessionTable = new JTable(sessionModel);
    TimeEntryTableModel timeEntryModel = new TimeEntryTableModel();
    JTable timeEntryTable = new JTable(timeEntryModel);
    JButton add = new JButton("+");
    JButton remove = new JButton("-");

    private JPanel createTimeEntryButtonPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets(5, 5, 5, 5);

        panel.add(add, gbc);
        gbc.gridy++;
        panel.add(remove, gbc);
        gbc.gridy++;

        return panel;
    }

    Sessions(Window parent, Task task) {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        JPanel right = new JPanel(new GridBagLayout());
        right.add(new JScrollPane(timeEntryTable), gbc);
        gbc.gridx++;
        gbc.gridheight = 1;
        gbc.weightx = 0;
        gbc.fill = GridBagConstraints.NONE;

        right.add(createTimeEntryButtonPanel(), gbc);

        gbc.weighty = 0;

        add.setEnabled(false);
        remove.setEnabled(false);

        JSplitPane split = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
        split.setResizeWeight(0.75);
        split.setLeftComponent(new JScrollPane(sessionTable));
        split.setRightComponent(right);

        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

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
            SessionRow row = new SessionRow();
            row.start = session.startTime;
            row.stop = session.stopTime;
            row.timeEntry = new ArrayList<>(session.timeEntry);

            sessionModel.data.add(row);
            sessionModel.fireTableRowsInserted(sessionModel.data.size() - 1, sessionModel.data.size() - 1);
        }

        sessionTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        sessionTable.getSelectionModel().addListSelectionListener(e -> {
            timeEntryModel.data.clear();
            timeEntryModel.fireTableDataChanged();

            add.setEnabled(sessionTable.getSelectedRow() != -1);

            if (sessionTable.getSelectedRow() != -1) {
                SessionRow session = sessionModel.data.get(sessionTable.getSelectedRow());

                for (TimeData.TimeEntry entry : session.timeEntry) {
                    timeEntryModel.data.add(entry);
                    timeEntryModel.fireTableRowsInserted(timeEntryModel.data.size() - 1, timeEntryModel.data.size() - 1);
                }
            }
        });

        timeEntryTable.getSelectionModel().addListSelectionListener(e -> {
            remove.setEnabled(timeEntryTable.getSelectedRow() != -1);
        });

        add.addActionListener(e -> {
            TimeData.TimeEntry entry = TimeEntryDialog.display(parent, mainFrame.getTimeData());

            if (entry != null) {
                SessionRow session = sessionModel.data.get(sessionTable.getSelectedRow());

                boolean found = false;

                int row = 0;
                int sessionRow = 0;

                for (TimeData.TimeEntry timeEntry : session.timeEntry) {
                    if (timeEntry.category.equals(entry.category)) {
                        row = timeEntryModel.data.indexOf(timeEntry);
                        sessionRow = session.timeEntry.indexOf(timeEntry);
                        found = true;

                        break;
                    }
                }

                if (!found) {
                    session.timeEntry.add(entry);
                    timeEntryModel.data.add(entry);
                    timeEntryModel.fireTableRowsInserted(timeEntryModel.data.size() - 1, timeEntryModel.data.size() - 1);
                }
                else {
                    timeEntryModel.data.set(row, entry);
                    session.timeEntry.set(sessionRow, entry);
                    timeEntryModel.fireTableRowsUpdated(row, row);
                }
            }
        });

        remove.addActionListener(e -> {
            SessionRow session = sessionModel.data.get(sessionTable.getSelectedRow());

            TimeData.TimeEntry timeEntry = session.timeEntry.get(timeEntryTable.getSelectedRow());

            TimeData.TimeCode code = null;

            for (TimeData.TimeEntry entry : task.timeEntry) {
                if (entry.category.equals(timeEntry.category)) {
                    code = entry.code;
                    break;
                }
            }

            if (code == null) {
                Task parentTask = mainFrame.getTaskModel().getTask(task.parentID);

                while (parentTask != null) {
                    Optional<TimeData.TimeEntry> parentEntry = parentTask.timeEntry.stream()
                            .filter(parentTimeEntry -> parentTimeEntry.category.equals(timeEntry.category))
                            .findFirst();

                    if (parentEntry.isPresent()) {
                        code = parentEntry.get().code;

                        break;
                    }
                    else {
                        parentTask = mainFrame.getTaskModel().getTask(parentTask.parentID);
                    }
                }

                // if we didn't find a code, show unknown
                if (parentTask == null) {
                    code = new TimeData.TimeCode();
                    code.name = "Unknown";
                    code.id = 0;
                }
            }

            TimeData.TimeEntry newEntry = new TimeData.TimeEntry(timeEntry.category, code);

            session.timeEntry.set(timeEntryTable.getSelectedRow(), newEntry);

            timeEntryModel.fireTableRowsUpdated(timeEntryTable.getSelectedRow(), timeEntryTable.getSelectedRow());
        });
    }

    public void save(Task task, UpdateTask update) {
        update.sessions.clear();

        for (SessionRow row : sessionModel.data) {
            update.sessions.add(new TaskInfo.Session(row.start, row.stop, row.timeEntry));
        }
    }

    class SessionRow {
        Instant start;
        Optional<Instant> stop;
        List<TimeData.TimeEntry> timeEntry = new ArrayList<>();
    }

    class SessionTableModel extends AbstractTableModel {
        public List<SessionRow> data = new ArrayList<>();

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
            SessionRow row = data.get(rowIndex);
            if (columnIndex == 0) {
                return row.start;
            }
            return row.stop.orElse(null);
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
}
