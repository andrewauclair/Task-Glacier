package config;

import data.Task;
import data.TimeData;
import dialogs.UpdateSessionTimeCode;
import packets.UpdateTask;
import taskglacier.MainFrame;
import util.Icons;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.TableCellRenderer;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

class TimeEntry extends JPanel {
    private final TimeData timeData;

    JButton add = new JButton(Icons.addIcon16);
    JButton remove = new JButton(Icons.removeIcon16);

    TableModel model = new TableModel();
    JTable table = new JTable(model);

    TimeEntry(TaskConfig taskConfig, MainFrame mainFrame, Task task) {
        setLayout(new GridBagLayout());

        this.timeData = mainFrame.getTimeData();

        remove.setEnabled(false);

        remove.addActionListener(e -> {
            Row existingRow = model.data.get(table.getSelectedRow());

            model.data.remove(table.getSelectedRow());
            model.fireTableRowsDeleted(table.getSelectedRow(), table.getSelectedRow());

            TimeData.TimeCategory category = existingRow.category;
            TimeData.TimeCode code = null;

            // try to find a code for the category in a parent
            Task parent = mainFrame.getTaskModel().getTask(task.parentID);

            while (parent != null) {
                Optional<TimeData.TimeEntry> parentEntry = parent.timeEntry.stream()
                        .filter(timeEntry -> timeEntry.category == existingRow.category)
                        .findFirst();

                if (parentEntry.isPresent()) {
                    code = parentEntry.get().code;
                    break;
                }
                else {
                    parent = mainFrame.getTaskModel().getTask(parent.parentID);
                }
            }

            // if we didn't find a code, show unknown
            if (parent == null) {
                code = new TimeData.TimeCode();
                code.name = "Unknown";
                code.id = 0;
            }

            model.data.add(new Row(category, code, true));
            model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);

            UpdateSessionTimeCode updateCodes = new UpdateSessionTimeCode(taskConfig, taskConfig.sessions.sessionModel.data, category, code);
            updateCodes.setVisible(true);
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
            TimeData.TimeEntry entry = TimeEntryDialog.display(taskConfig, timeData);

            if (entry != null) {
                // if we already have a code from this category, remove it
                for (int i = 0; i < model.data.size(); i++) {
                    if (model.data.get(i).category == entry.category) {
                        model.data.remove(i);
                        model.fireTableRowsDeleted(i, i);
                        break;
                    }
                }
                model.data.add(new Row(entry.category, entry.code, false));
                model.fireTableRowsInserted(model.data.size() - 1, model.data.size() - 1);

                UpdateSessionTimeCode updateCodes = new UpdateSessionTimeCode(taskConfig, taskConfig.sessions.sessionModel.data, entry.category, entry.code);
                updateCodes.setVisible(true);
            }
        });

        table.getColumnModel().removeColumn(table.getColumnModel().getColumn(2));

        TableCellRenderer renderer = createRenderer();

        table.setDefaultRenderer(String.class, renderer);
        table.setDefaultRenderer(boolean.class, renderer);

        for (TimeData.TimeCategory category : timeData.getTimeCategories()) {
            TimeData.TimeCode code = null;
            boolean inherited = false;

            Optional<TimeData.TimeEntry> first = task.timeEntry.stream()
                    .filter(timeEntry -> timeEntry.category != null && timeEntry.category.equals(category))
                    .findFirst();

            if (first.isPresent()) {
                code = first.get().code;

                if (code == null) {
                    code = new TimeData.TimeCode();
                    code.name = "Unknown";
                    code.id = 0;
                    inherited = true;
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
                        code = parentEntry.get().code;
                        inherited = true;

                        break;
                    }
                    else {
                        parent = mainFrame.getTaskModel().getTask(parent.parentID);
                    }
                }

                // if we didn't find a code, show unknown
                if (parent == null) {
                    code = new TimeData.TimeCode();
                    code.name = "Unknown";
                    code.id = 0;
                    inherited = true;
                }
            }

            model.data.add(new Row(category, code, inherited));
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

    public void save(Task task, UpdateTask update) {
        if (hasChanges(task)) {
            update.timeEntry.clear();

            List<Row> data = model.data.stream()
                    .filter(row -> !row.inherited)
                    .toList();

            for (Row row : data) {
                update.timeEntry.add(new TimeData.TimeEntry(row.category, row.code));
            }
        }
    }

    class Row {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
        boolean inherited;

        public Row(TimeData.TimeCategory category, TimeData.TimeCode code, boolean inherited) {
            this.category = category;
            this.code = code;
            this.inherited = inherited;
        }
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
}
