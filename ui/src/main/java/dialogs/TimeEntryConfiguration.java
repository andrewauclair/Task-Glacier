package dialogs;

import data.Standards;
import data.TimeData;
import packets.*;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TimeEntryConfiguration extends JDialog {
    private final TimeData timeData;

    private DefaultTableModel categoriesModel = new DefaultTableModel(0, 1);
    private JTable categoriesTable = new JTable(categoriesModel);

    private JButton categoryAdd = new JButton("+");
    private JButton categoryRemove = new JButton("-");

    private JButton save = new JButton("Save");

    class TimeEntryInstance {
        TimeData.TimeCategory category;
        List<TimeData.TimeCode> codes;

        CodeTableModel codeTableModel = new CodeTableModel();
        JTable codesTable = new JTable(codeTableModel);

        JPanel panel;

        JButton addCode = new JButton("+");
        JButton removeCode = new JButton("-");
    }
    private final Map<String, TimeEntryInstance> instances = new HashMap<>();

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

    public TimeEntryConfiguration(MainFrame mainFrame) {
        this.timeData = mainFrame.getTimeData();
        setModalityType(ModalityType.APPLICATION_MODAL);
        setTitle("Time Entry Configuration");

        KeyStroke ESCAPE_KEY = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);

        InputMap inputMap = ((JComponent) getContentPane()).getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW);
        ActionMap actionMap = ((JComponent) getContentPane()).getActionMap();

        inputMap.put(ESCAPE_KEY, "escape");
        actionMap.put("escape", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                TimeEntryConfiguration.this.dispose();
            }
        });

        JSplitPane split = new JSplitPane();
        split.setLeftComponent(buildCategories());

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);

        stack.add(createBlankPanel(), "blank");

        split.setRightComponent(stack);

        categoryAdd.addActionListener(e -> {
            String name = JOptionPane.showInputDialog(this, "New Bugzilla Instance Name");

            TimeEntryInstance instance = new TimeEntryInstance();
            instances.put(name, instance);

            JPanel instancePanel = createInstance(instance);

            stack.add(instancePanel, name);

            categoriesModel.addRow(new String[] { name });
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

                categoriesModel.removeRow(categoriesTable.getSelectedRow());
                categoriesModel.fireTableRowsDeleted(categoriesTable.getSelectedRow(), categoriesTable.getSelectedRow());

                instances.remove(name);

                layout.show(stack, "blank");
            }
        });

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
            for (TimeEntryInstance instance : instances.values()) {

            }

            TimeEntryConfiguration.this.dispose();
        });

        for (BugzillaInfo info : MainFrame.bugzillaInfo.values()) {
            TimeEntryInstance instance = new TimeEntryInstance();

            instances.put(info.name, instance);

            stack.add(createInstance(instance), info.name);

            categoriesModel.addRow(new Object[] { info.name });
            categoriesModel.fireTableRowsInserted(categoriesModel.getRowCount() - 1, categoriesModel.getRowCount() - 1);

            if (categoriesModel.getRowCount() == 1) {
                categoriesTable.getSelectionModel().setSelectionInterval(categoriesModel.getRowCount() - 1, categoriesModel.getRowCount() - 1);
            }

            for (String groupBy : info) {
                instance.groupByModel.addRow(new Object[] { groupBy });
                instance.groupByModel.fireTableRowsInserted(instance.groupByModel.getRowCount() - 1, instance.groupByModel.getRowCount() - 1);
            }

            instance.labelModel.setRowCount(0);
            instance.labelModel.fireTableDataChanged();

            info.labelToField.forEach((label, field) -> {
                instance.labelModel.addRow(new Object[] { label, field });
                instance.labelModel.fireTableRowsInserted(instance.labelModel.getRowCount() - 1, instance.labelModel.getRowCount() - 1);
            });
        }



        pack();

        setSize(400, 300);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    private JPanel buildCategories() {

        categoriesTable.setTableHeader(null);

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
//        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;

        instance.panel = panel;

        JButton groupByAdd = new JButton("+");
        JButton groupByRemove = new JButton("-");

        groupByAdd.addActionListener(e -> {
            String newCode = JOptionPane.showInputDialog(this, "New Time Code");

            instance.codeTableModel.rows.add(new TimeData.TimeCode(0, newCode));
            instance.codeTableModel.fireTableRowsInserted(instance.codeTableModel.getRowCount() - 1, instance.codeTableModel.getRowCount() - 1);
        });

        groupByRemove.addActionListener(e -> {
            instance.codeTableModel.rows.remove(instance.codesTable.getSelectedRow());
            instance.codeTableModel.fireTableRowsInserted(instance.codesTable.getSelectedRow(), instance.codesTable.getSelectedRow());
        });

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

    private JScrollPane setupTimeCategoriesTable() {
//        categoriesModel = new DefaultTableModel(new Object[] { "Time Category", "Label", "In Use", "Count", "Archived" }, 0);

        JTable categories = new JTable(categoriesModel);

        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem add = new JMenuItem("Add...");

        contextMenu.add(add);

        class CreateTimeCategory extends JDialog {
            CreateTimeCategory() {
                setModal(true);

                JTextField category = new JTextField();
                JTextField label = new JTextField();

                JButton add = new JButton("Add");
                add.addActionListener(e -> {
//                    categoriesModel.addRow(new Object[] { category.getText(), label.getText(), "No", "0", "No" });
                    TimeData.TimeCategory timeCategory = new TimeData.TimeCategory();
                    timeCategory.name = category.getText();
                    timeCategory.label = label.getText();
                    categoriesModel.rows.add(timeCategory);
                    categoriesModel.fireTableRowsInserted(categoriesModel.rows.size() - 1, categoriesModel.rows.size() - 1);

                    timeCodeModels.put(category.getText(), new CodeTableModel());
                    timeCategorySelection.addItem(category.getText());
                    CreateTimeCategory.this.dispose();
                });

                setLayout(new FlowLayout());

                add(new JLabel("Category"));
                add(category);
                add(new JLabel("Label"));
                add(label);
                add(add);

                pack();

                setLocationRelativeTo(TimeEntryConfiguration.this);
            }
        }
        add.addActionListener(e -> new CreateTimeCategory().setVisible(true));

        categories.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(categories, e.getX(), e.getY());
                }
            }
        });

        categories.setFillsViewportHeight(true);

        return new JScrollPane(categories);
    }

    private JPanel setupCodes() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.fill = GridBagConstraints.NONE;

        panel.add(timeCategorySelection, gbc);

        JTable codes = new JTable(null);

        timeCategorySelection.addItemListener(e -> {
            currentTimeCodeModel = timeCodeModels.get((String) e.getItem());
            codes.setModel(currentTimeCodeModel);
        });

        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem add = new JMenuItem("Add...");

        contextMenu.add(add);

        class CreateTimeCode extends JDialog {
            CreateTimeCode() {
                setModal(true);

                JTextField code = new JTextField();

                JButton add = new JButton("Add");
                add.addActionListener(e -> {
//                    currentTimeCodeModel.addRow(new Object[] { code.getText(), "No", "0", "No" });
                    TimeData.TimeCode timeCode = new TimeData.TimeCode();
                    timeCode.name = code.getText();
                    currentTimeCodeModel.rows.add(timeCode);
                    currentTimeCodeModel.fireTableRowsInserted(currentTimeCodeModel.rows.size() - 1, currentTimeCodeModel.rows.size() - 1);
                    CreateTimeCode.this.dispose();
                });

                setLayout(new FlowLayout());

                add(new JLabel("Code"));
                add(code);
                add(add);

                pack();

                setLocationRelativeTo(TimeEntryConfiguration.this);
            }
        }
        add.addActionListener(e -> new CreateTimeCode().setVisible(true));

        codes.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(codes, e.getX(), e.getY());
                }
            }
        });

        codes.setFillsViewportHeight(true);

        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.gridy++;
        panel.add(new JScrollPane(codes), gbc);

        return panel;
    }
}
