package dialogs;

import data.Standards;
import data.TimeData;
import packets.PacketType;
import packets.RequestID;
import packets.TimeCategoriesMessage;
import packets.TimeCategoryModType;
import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TimeEntryConfiguration extends JDialog {
    private final TimeData timeData;
    private final Map<String, TimeEntryInstance> instances = new HashMap<>();
    private CategoryTableModel categoriesModel = new CategoryTableModel();
    private JTable categoriesTable = new JTable(categoriesModel);

    private JButton categoryAdd = new JButton("+");
    private JButton categoryRemove = new JButton("-");

    private JButton save = new JButton("Save");

    public TimeEntryConfiguration(MainFrame mainFrame) {
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
            categoryRemove.setEnabled(categoriesTable.getSelectedRow() != -1);

            String name = categoriesTable.getSelectedRow() != -1 ? (String) categoriesModel.getValueAt(categoriesTable.getSelectedRow(), 0) : "blank";

            layout.show(stack, name);
        });

        categoryRemove.setEnabled(false);

        categoryRemove.addActionListener(e -> {
            if (categoriesTable.getSelectedRow() != -1) {
                String name = (String) categoriesModel.getValueAt(categoriesTable.getSelectedRow(), 0);

                categoriesModel.rows.remove(categoriesTable.getSelectedRow());
                categoriesModel.fireTableRowsDeleted(categoriesTable.getSelectedRow(), categoriesTable.getSelectedRow());

                instances.remove(name);

                layout.show(stack, "blank");
            }
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
            TimeCategoriesMessage addMessage = new TimeCategoriesMessage(PacketType.TIME_CATEGORIES_MODIFY);
            addMessage.type = TimeCategoryModType.ADD;

            TimeCategoriesMessage updateMessage = new TimeCategoriesMessage(PacketType.TIME_CATEGORIES_MODIFY);
            updateMessage.type = TimeCategoryModType.UPDATE;

            for (int i = 0; i < categoriesModel.getRowCount(); i++) {
                TimeData.TimeCategory timeCategory = new TimeData.TimeCategory();
                timeCategory.id = (int) categoriesModel.getValueAt(i, 1);
                timeCategory.name = (String) categoriesModel.getValueAt(i, 0);

                CodeTableModel timeCodeModel = instances.get(timeCategory.name).codeTableModel;

                boolean newTimeCodes = false;

                for (int j = 0; j < timeCodeModel.getRowCount(); j++) {
                    TimeData.TimeCode timeCode = new TimeData.TimeCode();
                    timeCode.id = (int) timeCodeModel.getValueAt(j, 1);
                    timeCode.name = (String) timeCodeModel.getValueAt(j, 0);

                    if (timeCode.id == 0) {
                        newTimeCodes = true;
                    }
                    timeCategory.timeCodes.add(timeCode);
                }

                if (timeCategory.id == 0 || newTimeCodes) {
                    addMessage.getTimeCategories().add(timeCategory);
                }
                else {
                    TimeData.TimeCategory category = mainFrame.getTimeData().findTimeCategory(timeCategory.id);

                    if (!category.name.equals(timeCategory.name)) {
                        updateMessage.getTimeCategories().add(timeCategory);
                    }
                }
            }

            if (!addMessage.getTimeCategories().isEmpty()) {
                addMessage.requestID = RequestID.nextRequestID();
                mainFrame.getConnection().sendPacket(addMessage);
            }

            if (!updateMessage.getTimeCategories().isEmpty()) {
                updateMessage.requestID = RequestID.nextRequestID();
                mainFrame.getConnection().sendPacket(updateMessage);
            }

            TimeEntryConfiguration.this.dispose();
        });

        for (TimeData.TimeCategory category : MainFrame.mainFrame.getTimeData().getTimeCategories()) {
            TimeEntryInstance instance = new TimeEntryInstance();
            instances.put(category.name, instance);

            stack.add(createInstance(instance), category.name);

            for (TimeData.TimeCode code : category.timeCodes) {
                instance.codeTableModel.rows.add(code);
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
        panel.add(categoryRemove, gbc);

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

        instance.removeCode.setEnabled(false);

        instance.addCode.addActionListener(e -> {
            String name = JOptionPane.showInputDialog(this, "New Time Code Name");

            // TODO prevent duplicates
            instance.codeTableModel.rows.add(new TimeData.TimeCode(0, name));
            instance.codeTableModel.fireTableRowsInserted(instance.codeTableModel.getRowCount() - 1, instance.codeTableModel.getRowCount() - 1);
        });
        instance.removeCode.addActionListener(e -> {
            instance.codeTableModel.rows.remove(instance.codesTable.getSelectedRow());
            instance.codeTableModel.fireTableRowsDeleted(instance.codesTable.getSelectedRow(), instance.codesTable.getSelectedRow());
        });
        instance.codesTable.getSelectionModel().addListSelectionListener(e -> {
            instance.removeCode.setEnabled(instance.codesTable.getSelectedRow() != -1);
        });
        JPanel buttons = new JPanel(new GridBagLayout());

        buttons.add(instance.addCode, gbc);
        gbc.gridy++;
        buttons.add(instance.removeCode, gbc);

        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;
        panel.add(new JScrollPane(instance.codesTable), gbc);

        gbc.gridx++;
        gbc.weightx = 0;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.NONE;
        panel.add(buttons, gbc);

        instance.panel = panel;

        return panel;
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

        JButton addCode = new JButton("+");
        JButton removeCode = new JButton("-");

        public TimeEntryInstance() {
            // hide ID column
            codesTable.removeColumn(codesTable.getColumnModel().getColumn(1));
        }
    }

    class CodeTableModel extends AbstractTableModel {
        List<TimeData.TimeCode> rows = new ArrayList<>();

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
            TimeData.TimeCode row = rows.get(rowIndex);

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
