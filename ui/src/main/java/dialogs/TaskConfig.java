package dialogs;

import data.ServerConnection;
import data.Task;
import data.TimeData;
import packets.RequestID;
import packets.TaskInfo;
import packets.UpdateTask;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableCellRenderer;
import java.awt.*;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class TaskConfig extends JDialog {
    // general (id, name, status, parent, bugzilla)
    class General extends JPanel {
        JTextField name = new JTextField(15);
        JTextField parent = new JTextField(3);
        JCheckBox serverControlled = new JCheckBox("Server Controlled");
        JCheckBox locked = new JCheckBox("Locked");

        General(Task task) {
            setLayout(new GridBagLayout());

            GridBagConstraints gbc = new GridBagConstraints();
            gbc.anchor = GridBagConstraints.NORTHWEST;
            gbc.insets = new Insets(5, 5, 5, 5);
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.weightx = 1;
            gbc.weighty = 0;

            add(new JLabel("ID " + task.id), gbc);

            gbc.gridy++;
            add(new LabeledComponent("Name", name), gbc);

            name.setText(task.name);

            JComboBox<String> status = new JComboBox<>();
            status.addItem("Inactive");
            status.addItem("Active");
            status.addItem("Finished");

            gbc.gridy++;
            add(new LabeledComponent("Status", status), gbc);

            parent.setText(String.valueOf(task.parentID));

            gbc.gridy++;
            add(new LabeledComponent("Parent", parent), gbc);


            serverControlled.setEnabled(false);
            serverControlled.setSelected(task.serverControlled);

            gbc.gridy++;
            add(serverControlled, gbc);

            locked.setSelected(task.locked);

            gbc.gridy++;
            add(locked, gbc);

            // add filler
            gbc.weighty = 1;
            gbc.fill = GridBagConstraints.BOTH;
            add(new JLabel(), gbc);

            locked.addActionListener(e -> {
                boolean controlsLocked = serverControlled.isSelected() || locked.isSelected();

                name.setEnabled(!controlsLocked);
                status.setEnabled(!controlsLocked);
                parent.setEnabled(!controlsLocked);
            });

            boolean controlsLocked = serverControlled.isSelected() || locked.isSelected();

            name.setEnabled(!controlsLocked);
            status.setEnabled(!controlsLocked);
            parent.setEnabled(!controlsLocked);

        }

        private boolean hasChanges(Task task) {
            return !task.name.equals(name.getText()) ||
                    task.parentID != Integer.parseInt(parent.getText()) ||
                    task.locked != locked.isSelected();
        }

        public void save(Task task, ServerConnection connection) {
            if (hasChanges(task)) {
                // send packet
                UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task.id, Integer.parseInt(parent.getText()), name.getText());
                update.locked = locked.isSelected();
                connection.sendPacket(update);
            }
        }
    }

    class Labels extends JPanel {
        Labels(Task task) {
            add(new JLabel(getClass().toGenericString()));
        }
    }

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

    class TimeEntry extends JPanel {
        private final TimeData timeData;

        JButton add = new JButton("+");
        JButton remove = new JButton("-");

        TableModel model = new TableModel();
        JTable table = new JTable(model);

        class Row {
            TimeData.TimeCategory category;
            TimeData.TimeCode code;
            boolean inherited;
        }

        class TableModel extends AbstractTableModel {
            public List<Row> data = new ArrayList<>();

            @Override
            public int getRowCount() {
                return data.size();
            }

            @Override
            public String getColumnName(int column) {
                if (column == 0) {
                    return "Category";
                }
                else if (column == 1) {
                    return "Code";
                }
                return "Inherited";
            }

            @Override
            public Class<?> getColumnClass(int columnIndex) {
                return String.class;
            }

            @Override
            public int getColumnCount() {
                return 3;
            }

            @Override
            public Object getValueAt(int rowIndex, int columnIndex) {
                Row row = data.get(rowIndex);
                if (columnIndex == 0) {
                    return row.category.name;
                }
                if (columnIndex == 1) {
                    return row.code.name;
                }
                return row.inherited;
            }
        }

        class TimeEntryDialog extends JDialog {
            TimeEntryDialog() {
                super(TaskConfig.this, true);

                setLayout(new GridBagLayout());

                JButton add = new JButton("Add");
                add.setEnabled(false);

                JComboBox<String> timeCategory = new JComboBox<>();
                JComboBox<String> timeCode = new JComboBox<>();

                timeCategory.addActionListener(e -> {
                    Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                            .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                            .findFirst();

                    timeCode.removeAllItems();
                    add.setEnabled(false);

                    if (timeCategory2.isPresent()) {
                        for (TimeData.TimeCode code : timeCategory2.get().timeCodes) {
                            timeCode.addItem(code.name);
                        }
                        timeCode.setSelectedItem(null);
                    }
                });

                timeCode.addActionListener(e -> add.setEnabled(true));
                for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
                    timeCategory.addItem(category.name);
                }

                timeCategory.setSelectedItem(null);

                GridBagConstraints gbc = new GridBagConstraints();
                gbc.anchor = GridBagConstraints.NORTHWEST;
                gbc.insets = new Insets(5, 5, 5, 5);
                gbc.gridx = 0;
                gbc.gridy = 0;

                add(new LabeledComponent("Time Category", timeCategory), gbc);
                gbc.gridy++;
                add(new LabeledComponent("Time Code", timeCode), gbc);
                gbc.gridy++;
                gbc.anchor = GridBagConstraints.CENTER;
                add(add, gbc);

                add.addActionListener(e -> {
                    Optional<TimeData.TimeCategory> timeCategory2 = timeData.getTimeCategories().stream()
                            .filter(timeCategory1 -> timeCategory1.name.equals(timeCategory.getSelectedItem()))
                            .findFirst();

                    Row row = new Row();
                    row.category = timeCategory2.get();
                    row.code = timeCategory2.get().timeCodes.stream()
                            .filter(timeCode1 -> timeCode1.name.equals(timeCode.getSelectedItem()))
                            .findFirst().get();

                    // if we already have a code from this category, remove it
                    for (int i = 0; i < model.data.size(); i++) {
                        if (model.data.get(i).category == row.category) {
                            model.data.remove(i);
                            model.fireTableRowsDeleted(i, i);
                            break;
                        }
                    }
                    model.data.add(row);
                    model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);

                    dispose();
                });

                pack();

                setLocationRelativeTo(TaskConfig.this);
            }
        }

        TimeEntry(MainFrame mainFrame, Task task) {
            setLayout(new GridBagLayout());

            this.timeData = mainFrame.getTimeData();

            remove.setEnabled(false);

            remove.addActionListener(e -> {
                Row existingRow = model.data.get(table.getSelectedRow());

                model.data.remove(table.getSelectedRow());
                model.fireTableRowsDeleted(table.getSelectedRow(), table.getSelectedRow());

                Row row = new Row();
                row.category = existingRow.category;

                // try to find a code for the category in a parent
                Task parent = mainFrame.getTaskModel().getTask(task.parentID);

                while (parent != null) {
                    Optional<TimeData.TimeEntry> parentEntry = parent.timeEntry.stream()
                            .filter(timeEntry -> timeEntry.category == existingRow.category)
                            .findFirst();

                    if (parentEntry.isPresent()) {
                        row.code = parentEntry.get().code;
                        row.inherited = true;

                        break;
                    }
                    else {
                        parent = mainFrame.getTaskModel().getTask(parent.parentID);
                    }
                }

                // if we didn't find a code, show unknown
                if (parent == null) {
                    row.code = new TimeData.TimeCode();
                    row.code.name = "Unknown";
                    row.code.id = 0;
                    row.inherited = true;
                }

                model.data.add(row);
                model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);
            });

            JPanel buttons = new JPanel(new FlowLayout());
            buttons.add(add);
            buttons.add(remove);

            GridBagConstraints gbc = new GridBagConstraints();
            gbc.gridx = 0;
            gbc.gridy = 0;

            add(buttons, gbc);

            gbc.gridy++;
            gbc.weightx = 1;
            gbc.weighty = 1;
            gbc.fill = GridBagConstraints.BOTH;

            add(new JScrollPane(table), gbc);

            table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

            table.getSelectionModel().addListSelectionListener(e -> {
                remove.setEnabled(table.getSelectedRow() != -1);
            });

            add.addActionListener(e -> {
                TimeEntryDialog dialog = new TimeEntryDialog();
                dialog.setVisible(true);
                dialog.setLocationRelativeTo(mainFrame);
            });

            table.getColumnModel().removeColumn(table.getColumnModel().getColumn(2));

            TableCellRenderer renderer = createRenderer();

            table.setDefaultRenderer(String.class, renderer);
            table.setDefaultRenderer(boolean.class, renderer);

            for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
                Row row = new Row();
                row.category = category;

                Optional<TimeData.TimeEntry> first = task.timeEntry.stream()
                        .filter(timeEntry -> timeEntry.category != null && timeEntry.category.equals(category))
                        .findFirst();

                if (first.isPresent()) {
                    row.code = first.get().code;

                    if (row.code == null) {
                        row.code = new TimeData.TimeCode();
                        row.code.name = "Unknown";
                        row.code.id = 0;
                        row.inherited = true;
                    }
                }
                else {
                    // try to find a code for the category in a parent
                    Task parent = mainFrame.getTaskModel().getTask(task.parentID);

                    while (parent != null) {
                        Optional<TimeData.TimeEntry> parentEntry = parent.timeEntry.stream()
                                .filter(timeEntry -> timeEntry.category.equals(category))
                                .findFirst();

                        if (parentEntry.isPresent()) {
                            row.code = parentEntry.get().code;
                            row.inherited = true;

                            break;
                        }
                        else {
                            parent = mainFrame.getTaskModel().getTask(parent.parentID);
                        }
                    }

                    // if we didn't find a code, show unknown
                    if (parent == null) {
                        row.code = new TimeData.TimeCode();
                        row.code.name = "Unknown";
                        row.code.id = 0;
                        row.inherited = true;
                    }
                }

                model.data.add(row);
                model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);
            }
        }

        private TableCellRenderer createRenderer() {
            DefaultTableCellRenderer defaultRenderer = new DefaultTableCellRenderer();

            TableCellRenderer renderer = (table, value, isSelected, hasFocus, row, column) -> {
                Component component = defaultRenderer.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);

                boolean inherited = (boolean) model.getValueAt(row, 2);

                if (inherited) {
                    Font font = component.getFont();
                    font = font.deriveFont(Font.ITALIC);

                    component.setFont(font);
                }
                return component;
            };
            return renderer;
        }

        private boolean hasChanges(Task task) {
            List<Row> data = model.data.stream()
                    .filter(row -> !row.inherited)
                    .toList();

            if (task.timeEntry.size() != data.size()) {
                return true;
            }
            for (int i = 0; i < task.timeEntry.size(); i++) {
                if (!task.timeEntry.get(i).category.equals(data.get(i).category) ||
                    !task.timeEntry.get(i).code.equals(data.get(i).code)) {
                    return true;
                }
            }
            return false;
        }

        public void save(Task task, ServerConnection connection) {
            if (hasChanges(task)) {
                UpdateTask update = new UpdateTask(RequestID.nextRequestID(), task);
                update.timeEntry.clear();

                List<Row> data = model.data.stream()
                        .filter(row -> !row.inherited)
                        .toList();

                for (Row row : data) {
                    TimeData.TimeEntry entry = new TimeData.TimeEntry();
                    entry.category = row.category;
                    entry.code = row.code;
                    update.timeEntry.add(entry);
                }
                connection.sendPacket(update);
            }
        }
    }

    public TaskConfig(MainFrame mainFrame, Task task) {
        super(mainFrame, true);

        setLayout(new GridBagLayout());

        setTitle("Task Configuration - " + task.name);

        DefaultTableModel model = new DefaultTableModel(0, 1);
        model.addRow(new Object[]{"General"});
        model.addRow(new Object[]{"Labels"});
        model.addRow(new Object[]{"Sessions"});
        model.addRow(new Object[]{"Time Entry"});

        JTable list = new JTable(model);
        list.setTableHeader(null);

        JPanel foo = new JPanel(new BorderLayout());
        JSplitPane split = new JSplitPane();
        split.setLeftComponent(new JScrollPane(list));

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);

        stack.add(new JPanel(), "");
        General general = new General(task);
        stack.add(general, "General");
        stack.add(new Labels(task), "Labels");
        stack.add(new Sessions(task), "Sessions");
        TimeEntry timeEntry = new TimeEntry(mainFrame, task);
        stack.add(timeEntry, "Time Entry");
        split.setRightComponent(stack);

        SwingUtilities.invokeLater(() -> {
            split.setDividerLocation(0.3);
        });

        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        list.getSelectionModel().addListSelectionListener(e -> {
            if (e.getValueIsAdjusting()) {
                return;
            }
            String name = e.getFirstIndex() != -1 ? (String) model.getValueAt(list.getSelectedRow(), 0) : "";

            layout.show(stack, name);
        });

        foo.add(split);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridy = 0;
        gbc.gridx = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.insets = new Insets(5, 5, 5, 5);

        add(foo, gbc);

        gbc.gridy++;
        gbc.weighty = 0;
        gbc.weighty = 0;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.fill = GridBagConstraints.NONE;

        JButton save = new JButton("Save");

        add(save, gbc);

        save.addActionListener(e -> {
            // send any packets that are necessary
            general.save(task, mainFrame.getConnection());
            timeEntry.save(task, mainFrame.getConnection());
            dispose();
        });

        setSize(500, 350);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
