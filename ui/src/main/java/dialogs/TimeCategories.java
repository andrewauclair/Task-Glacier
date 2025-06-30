package dialogs;

import data.TimeData;
import packets.PacketType;
import packets.RequestID;
import packets.TimeCategoriesMessage;
import packets.TimeCategoryModType;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TimeCategories extends JDialog {
    private final TimeData timeData;
    JComboBox<String> timeCategorySelection = new JComboBox<>();
    Map<String, CodeTableModel> timeCodeModels = new HashMap<>();
    CodeTableModel currentTimeCodeModel = null;
    private CategoriesTableModel categoriesModel = new CategoriesTableModel();

    class Row {
        TimeData.TimeCategory category;
        TimeData.TimeCode code;
    }

    class CategoriesTableModel extends AbstractTableModel {
        List<TimeData.TimeCategory> rows = new ArrayList<>();

        @Override
        public int getRowCount() {
            return rows.size();
        }

        @Override
        public boolean isCellEditable(int rowIndex, int columnIndex) {
            return columnIndex == 0 || columnIndex == 1;
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            if (columnIndex == 0 || columnIndex == 1) {
                return String.class;
            }
            if (columnIndex == 3 || columnIndex == 5) {
                return int.class;
            }
            return boolean.class;
        }

        @Override
        public String getColumnName(int column) {
            switch (column) {
                case 0:
                    return "Time Category";
                case 1:
                    return "Label";
                case 2:
                    return "In Use";
                case 3:
                    return "Count";
                case 4:
                    return "Archived";
                case 5:
                    return "ID";
            }
            return null;
        }

        @Override
        public int getColumnCount() {
            return 6;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            TimeData.TimeCategory row = rows.get(rowIndex);

            switch (columnIndex) {
                case 0:
                    return row.name;
                case 1:
                    return row.label;
                case 2:
                    return false;
                case 3:
                    return 0;
                case 4:
                    return false;
                case 5:
                    return row.id;
            }
            return null;
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
                    return "In Use";
                case 2:
                    return "Task Count";
                case 3:
                    return "Archived";
                case 4:
                    return "ID";
            }
            return null;
        }

        @Override
        public boolean isCellEditable(int rowIndex, int columnIndex) {
            return columnIndex == 0;
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return super.getColumnClass(columnIndex);
        }

        @Override
        public int getColumnCount() {
            return 5;
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            TimeData.TimeCode row = rows.get(rowIndex);

            switch (columnIndex) {
                case 0:
                    return row.name;
                case 1:
                    return false;
                case 2:
                    return 0;
                case 3:
                    return false;
                case 4:
                    return row.id;
            }
            return null;
        }
    }
    public TimeCategories(MainFrame mainFrame) {
        this.timeData = mainFrame.getTimeData();
        setModal(true);

        // Time Category - Label - In Use - Count - Archived
        // Project Server - PS - Yes - 10 - No
        // SAP - SAP - Yes - 12 - No
        // Something - SO - No - 5 - Yes

        // Time Code - In Use - Task Count - Archived
        // G101 - Yes - 100 - No
        // G102 - No - 10 - Yes

        JTabbedPane tabs = new JTabbedPane();

        tabs.add("Categories", setupTimeCategoriesTable());
        tabs.add("Codes", setupCodes());

        for (TimeData.TimeCategory timeCategory : timeData.getTimeCategories()) {
//            categoriesModel.addRow(new Object[] { timeCategory.name, "", "No", "0", "No" });

            categoriesModel.rows.add(timeCategory);

            timeCodeModels.put(timeCategory.name, new CodeTableModel());
            CodeTableModel timeCodeModel = timeCodeModels.get(timeCategory.name);

            timeCategorySelection.addItem(timeCategory.name);

            for (TimeData.TimeCode timeCode : timeCategory.timeCodes) {
//                timeCodeModel.addRow(new Object[] { timeCode.name, "No", "0", "No" });
                timeCodeModel.rows.add(timeCode);
            }
        }

        JButton save = new JButton("Save");

        save.addActionListener(e -> {
            TimeCategoriesMessage addMessage = new TimeCategoriesMessage(PacketType.TIME_CATEGORIES_MODIFY);
            addMessage.type = TimeCategoryModType.ADD;

            TimeCategoriesMessage updateMessage = new TimeCategoriesMessage(PacketType.TIME_CATEGORIES_MODIFY);
            updateMessage.type = TimeCategoryModType.UPDATE;

            for (int i = 0; i < categoriesModel.getRowCount(); i++) {
                TimeData.TimeCategory timeCategory = new TimeData.TimeCategory();
                timeCategory.id = (int) categoriesModel.getValueAt(i, 5);
                timeCategory.name = (String) categoriesModel.getValueAt(i, 0);
                timeCategory.label = (String) categoriesModel.getValueAt(i, 1);

                CodeTableModel timeCodeModel = timeCodeModels.get(timeCategory.name);

                boolean newTimeCodes = false;

                for (int j = 0; j < timeCodeModel.getRowCount(); j++) {
                    TimeData.TimeCode timeCode = new TimeData.TimeCode();
                    timeCode.id = (int) timeCodeModel.getValueAt(j, 4);
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

                    if (!category.name.equals(timeCategory.name) || !category.label.equals(timeCategory.label)) {
                        updateMessage.getTimeCategories().add(timeCategory);
//                        message.type = TimeCategoryModType.UPDATE;
//                        message.requestID = RequestID.nextRequestID();
//                        mainFrame.getConnection().sendPacket(message);
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

            TimeCategories.this.dispose();
        });

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 1;
        gbc.weighty = 1;
        add(tabs, gbc);
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        gbc.gridy++;
        gbc.anchor = GridBagConstraints.CENTER;
        add(save, gbc);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
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

                setLocationRelativeTo(TimeCategories.this);
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

                setLocationRelativeTo(TimeCategories.this);
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
