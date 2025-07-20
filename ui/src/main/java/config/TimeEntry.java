package config;

import data.ServerConnection;
import data.Task;
import data.TimeData;
import packets.RequestID;
import packets.UpdateTask;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.TableCellRenderer;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

class TimeEntry extends JPanel {
    private final TaskConfig taskConfig;
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
            } else if (column == 1) {
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
            super(taskConfig, true);

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

            setLocationRelativeTo(taskConfig);
        }
    }

    TimeEntry(TaskConfig taskConfig, MainFrame mainFrame, Task task) {
        this.taskConfig = taskConfig;
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
                } else {
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
            } else {
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
                    } else {
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
            update.locked = task.locked;
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
