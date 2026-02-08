package dialogs;

import data.Standards;
import data.TimeData;
import packets.RequestID;
import packets.TimeEntryModify;
import packets.TimeCategoryModType;
import taskglacier.MainFrame;
import util.DialogEscape;
import util.Icons;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableRowSorter;
import java.awt.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TimeEntryConfiguration extends JDialog {
    private final TimeData timeData;
    private final Map<String, TimeEntryInstance> instances = new HashMap<>();
    private CategoryTableModel categoriesModel = new CategoryTableModel();
    private JTable categoriesTable = new JTable(categoriesModel);

    private JButton categoryAdd = new JButton(Icons.addIcon16);

    private JButton save = new JButton("Save");

    public TimeEntryConfiguration(MainFrame mainFrame) {
        super(mainFrame);

        this.timeData = mainFrame.getTimeData();

        setModalityType(ModalityType.APPLICATION_MODAL);

        setTitle("Time Entry Configuration");

        DialogEscape.addEscapeHandler(this);

        JSplitPane split = new JSplitPane();
        split.setLeftComponent(buildCategories());

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);

        stack.add(createBlankPanel(), "blank");

        split.setRightComponent(stack);

        categoryAdd.addActionListener(e -> {
            String name = JOptionPane.showInputDialog(this, "New Time Category Name");

            TimeEntryInstance instance = new TimeEntryInstance();
            instances.put(name, instance);

            JPanel instancePanel = createInstance(instance);

            stack.add(instancePanel, name);

            categoriesModel.rows.add(new TimeData.TimeCategory(name, 0));
            categoriesModel.fireTableRowsInserted(categoriesModel.getRowCount() - 1, categoriesModel.getRowCount() - 1);

            categoriesTable.getSelectionModel().setSelectionInterval(categoriesModel.getRowCount() - 1, categoriesModel.getRowCount() - 1);
        });

        categoriesTable.getSelectionModel().addListSelectionListener(e -> {
            String name = categoriesTable.getSelectedRow() != -1 ? (String) categoriesModel.getValueAt(categoriesTable.getSelectedRow(), 0) : "blank";

            layout.show(stack, name);
        });

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(split, gbc);

        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.gridy++;

        add(save, gbc);

        save.addActionListener(e -> {
            TimeEntryModify message = new TimeEntryModify();

            for (int i = 0; i < categoriesModel.getRowCount(); i++) {
                TimeEntryModify.Category timeCategory = new TimeEntryModify.Category();
                timeCategory.id = (int) categoriesModel.getValueAt(i, 1);
                timeCategory.name = (String) categoriesModel.getValueAt(i, 0);

                CodeTableModel timeCodeModel = instances.get(timeCategory.name).codeTableModel;

                boolean categoryAdded = timeCategory.id == 0;
                int categoryIndex = 0;

                if (timeCategory.id == 0) {
                    timeCategory.type = TimeCategoryModType.ADD;
                    message.categories.add(timeCategory);
                    categoryIndex = message.categories.size() - 1;
                }
                else {
                    TimeData.TimeCategory category = mainFrame.getTimeData().findTimeCategory(timeCategory.id);

                    if (!category.name.equals(timeCategory.name)) {
                        timeCategory.type = TimeCategoryModType.UPDATE;
                        message.categories.add(timeCategory);
                        categoryIndex = message.categories.size() - 1;
                    }
                }

                for (int j = 0; j < timeCodeModel.getRowCount(); j++) {
                    TableTimeCode row = timeCodeModel.rows.get(j);

                    if (!categoryAdded && (row.newCode || row.modified)) {
                        categoryAdded = true;
                        timeCategory.type = TimeCategoryModType.UPDATE;
                        message.categories.add(timeCategory);
                        categoryIndex = message.categories.size() - 1;
                    }

                    if (row.newCode || row.modified) {
                        TimeEntryModify.Code newCode = new TimeEntryModify.Code();

                        newCode.type = row.newCode ? TimeCategoryModType.ADD : TimeCategoryModType.UPDATE;
                        newCode.categoryIndex = categoryIndex;
                        newCode.id = row.id;
                        newCode.name = row.name;
                        newCode.archived = row.archived;

                        message.codes.add(newCode);
                    }
                }
            }

            if (!message.categories.isEmpty()) {
                message.requestID = RequestID.nextRequestID();
                mainFrame.getConnection().sendPacket(message);
            }

            TimeEntryConfiguration.this.dispose();
        });

        for (TimeData.TimeCategory category : MainFrame.mainFrame.getTimeData().getTimeCategories()) {
            TimeEntryInstance instance = new TimeEntryInstance();
            instances.put(category.name, instance);

            stack.add(createInstance(instance), category.name);

            for (TimeData.TimeCode code : category.timeCodes) {
                TableTimeCode tableCode = new TableTimeCode(code.id, code.name, code.archived);
                instance.codeTableModel.rows.add(tableCode);
            }
            instance.codeTableModel.fireTableDataChanged();

            categoriesModel.rows.add(category);
            categoriesModel.fireTableRowsInserted(categoriesModel.getRowCount() - 1, categoriesModel.getRowCount() - 1);

            if (categoriesModel.getRowCount() == 1) {
                categoriesTable.getSelectionModel().setSelectionInterval(categoriesModel.getRowCount() - 1, categoriesModel.getRowCount() - 1);
            }
        }

        pack();

        setSize(400, 300);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    private JPanel buildCategories() {
        categoriesTable.setTableHeader(null);
        categoriesTable.removeColumn(categoriesTable.getColumnModel().getColumn(1));

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        gbc.gridwidth = 1;

        JPanel panel = new JPanel(new GridBagLayout());

        panel.add(categoryAdd, gbc);
        gbc.gridx++;
        panel.add(new JLabel("      "), gbc);

        gbc.gridx = 0;
        gbc.gridy++;
        gbc.gridwidth = 2;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        categoriesTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        panel.add(categoriesTable, gbc);

        return panel;
    }

    private JPanel createInstance(TimeEntryInstance instance) {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();

        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        instance.editCode.addActionListener(e -> {
            int row = instance.codesTable.convertRowIndexToModel(instance.codesTable.getSelectedRow());
            String currentName = (String) instance.codeTableModel.getValueAt(row, 0);
            String name = JOptionPane.showInputDialog(this, "Edit Time Code Name", currentName);

            TableTimeCode existingCode = instance.codeTableModel.rows.get(row);
            existingCode.name = name;
            existingCode.modified = true;
            instance.codeTableModel.fireTableRowsUpdated(row, row);
        });
        instance.addCode.addActionListener(e -> {
            String name = JOptionPane.showInputDialog(this, "New Time Code Name");

            TableTimeCode newCode = new TableTimeCode(0, name, false);
            newCode.newCode = true;
            instance.codeTableModel.rows.add(newCode);
            instance.codeTableModel.fireTableRowsInserted(instance.codeTableModel.getRowCount() - 1, instance.codeTableModel.getRowCount() - 1);
        });
        instance.archiveDown.addActionListener(e -> {
            int row = instance.codesTable.convertRowIndexToModel(instance.codesTable.getSelectedRow());
            TableTimeCode code = instance.codeTableModel.rows.get(row);

            code.modified = true;
            code.archived = true;
            instance.codeTableModel.fireTableRowsUpdated(row, row);
        });
        instance.archiveUp.addActionListener(e -> {
            int row = instance.codesTable.convertRowIndexToModel(instance.codesTable.getSelectedRow());
            TableTimeCode code = instance.codeTableModel.rows.get(row);

            code.modified = true;
            code.archived = false;
            instance.codeTableModel.fireTableRowsUpdated(row, row);
        });
        instance.archiveToggle.addActionListener(e -> {
            createSorter(instance);

            instance.addCode.setEnabled(!instance.archiveToggle.isSelected());
        });

        createSorter(instance);

        instance.codesTable.getSelectionModel().addListSelectionListener(e -> {
            instance.editCode.setEnabled(instance.codesTable.getSelectedRow() != -1);

            instance.archiveDown.setEnabled(false);
            instance.archiveUp.setEnabled(false);

            if (instance.codesTable.getSelectedRow() != -1) {
                TableTimeCode code = instance.codeTableModel.rows.get(instance.codesTable.convertRowIndexToModel(instance.codesTable.getSelectedRow()));

                instance.archiveDown.setEnabled(!code.archived);
                instance.archiveUp.setEnabled(code.archived);
            }
            instance.editCode.setEnabled(instance.codesTable.getSelectedRow() != -1);
        });

        JPanel buttons = new JPanel(new GridBagLayout());

        instance.editCode.setEnabled(false);
        instance.archiveDown.setEnabled(false);
        instance.archiveUp.setEnabled(false);

        buttons.add(instance.editCode, gbc);
        gbc.gridy++;
        buttons.add(instance.addCode, gbc);
        gbc.gridy++;
        buttons.add(instance.archiveDown, gbc);
        gbc.gridy++;
        buttons.add(instance.archiveUp, gbc);

        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        panel.add(new JScrollPane(instance.codesTable), gbc);

        gbc.gridx++;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        panel.add(buttons, gbc);

        gbc.gridx = 0;
        gbc.gridy++;

        {
            GridBagConstraints gbcNested = new GridBagConstraints();

            gbcNested.anchor = GridBagConstraints.NORTHWEST;
            gbcNested.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
            gbcNested.gridx = 0;
            gbcNested.gridy = 0;

            JPanel bottom = new JPanel(new GridBagLayout());

            gbcNested.fill = GridBagConstraints.HORIZONTAL;
            gbcNested.weightx = 1.0;

            bottom.add(instance.searchText, gbcNested);

            gbcNested.gridx++;
            gbcNested.fill = GridBagConstraints.NONE;
            gbcNested.weightx = 0;

            bottom.add(instance.archiveToggle, gbcNested);

            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.weightx = 1.0;

            panel.add(bottom, gbc);
        }
        instance.panel = panel;

        return panel;
    }

    private static void createSorter(TimeEntryInstance instance) {
        TableRowSorter<CodeTableModel> sorter = new TableRowSorter<>(instance.codeTableModel);

        sorter.setRowFilter(new RowFilter<>() {
            @Override
            public boolean include(Entry<? extends CodeTableModel, ? extends Integer> entry) {
                boolean archived = entry.getModel().rows.get(entry.getIdentifier()).archived;

                if (instance.archiveToggle.isSelected()) {
                    return archived;
                }
                return !archived;
            }
        });
        sorter.setSortsOnUpdates(true);

        instance.codesTable.setRowSorter(sorter);
    }

    private JPanel createBlankPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        panel.add(new JLabel(), gbc);

        return panel;
    }

    class CategoryTableModel extends AbstractTableModel {
        List<TimeData.TimeCategory> rows = new ArrayList<>();

        @Override
        public int getRowCount() {
            return rows.size();
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Time Category";
                case 1:
                    return "ID";
            }
            return null;
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return super.getColumnClass(columnIndex);
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            TimeData.TimeCategory row = rows.get(rowIndex);

            switch (columnIndex) {
                case 0:
                    return row.name;
                case 1:
                    return row.id;
            }
            return null;
        }
    }

    class TimeEntryInstance {
        TimeData.TimeCategory category;
        List<TimeData.TimeCode> codes;

        CodeTableModel codeTableModel = new CodeTableModel();
        JTable codesTable = new JTable(codeTableModel);

        JPanel panel;

        JButton editCode = new JButton(Icons.editIcon16);
        JButton addCode = new JButton(Icons.addIcon16);

        JButton archiveDown = new JButton(Icons.archiveDown16);
        JButton archiveUp = new JButton(Icons.archiveUp16);

        JToggleButton archiveToggle = new JToggleButton(Icons.archiveToggle16);

        JTextField searchText = new JTextField();

        public TimeEntryInstance() {
            // hide ID column
            codesTable.removeColumn(codesTable.getColumnModel().getColumn(1));
        }
    }

    class TableTimeCode {
        public int id;
        public String name;
        public boolean archived;

        public boolean newCode = false;
        public boolean modified = false;

        public TableTimeCode(int id, String name, boolean archived) {
            this.id = id;
            this.name = name;
            this.archived = archived;
        }
    }

    class CodeTableModel extends AbstractTableModel {
        List<TableTimeCode> rows = new ArrayList<>();

        @Override
        public int getRowCount() {
            return rows.size();
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Time Code";
                case 1:
                    return "ID";
            }
            return null;
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return super.getColumnClass(columnIndex);
        }

        @Override
        public int getColumnCount() {
            return 2;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            TableTimeCode row = rows.get(rowIndex);

            switch (columnIndex) {
                case 0:
                    return row.name;
                case 1:
                    return row.id;
            }
            return null;
        }
    }
}
